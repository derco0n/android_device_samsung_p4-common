/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_hw_tegra"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include <hardware/audio.h>
#include <hardware/hardware.h>
#include <hardware/audio_effect.h>

#include <system/audio.h>

#include <tinyalsa/asoundlib.h>

#include <audio_utils/resampler.h>
#include <audio_utils/echo_reference.h>
#include <audio_route/audio_route.h>
#include <audio_effects/effect_aec.h>

#include <dlfcn.h>

#include "secril-client.h"

// RILD required
// from system/core/include/utils/Errors.h
typedef int32_t status_t;
enum {
    OK = 0, // Everything's swell.
    NO_ERROR = 0, // No errors.
    INVALID_OPERATION = -ENOSYS,
};

#define PCM_CARD 0
#define PCM_DEVICE 0

#define MIXER_CARD 0

#define OUT_PERIOD_SIZE 1024
#define OUT_SHORT_PERIOD_COUNT 2
#define OUT_LONG_PERIOD_COUNT 4
#define OUT_SAMPLING_RATE 44100

#define IN_PERIOD_SIZE 1024
#define IN_PERIOD_SIZE_LOW_LATENCY 512
#define IN_PERIOD_COUNT 4
#define IN_SAMPLING_RATE 44100

/* minimum sleep time in out_write() when write threshold is not reached */
#define MIN_WRITE_SLEEP_US 2000
#define MAX_WRITE_SLEEP_US ((OUT_PERIOD_SIZE * OUT_SHORT_PERIOD_COUNT * 1000000) \
                                / OUT_SAMPLING_RATE)

/* from Tuna */
#define MAX_PREPROCESSORS 3 /* maximum one AGC + one NS + one AEC per input stream */

struct effect_info_s {
    effect_handle_t effect_itfe;
    size_t num_channel_configs;
    channel_config_t* channel_configs;
};

enum {
    OUT_BUFFER_TYPE_UNKNOWN,
    OUT_BUFFER_TYPE_SHORT,
    OUT_BUFFER_TYPE_LONG,
};

typedef enum {
    TTY_MODE_OFF,
    TTY_MODE_VCO,
    TTY_MODE_HCO,
    TTY_MODE_FULL
} tty_modes_t;

struct pcm_config pcm_config_out = {
    .channels = 2,
    .rate = OUT_SAMPLING_RATE,
    .period_size = OUT_PERIOD_SIZE,
    .period_count = OUT_LONG_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = OUT_PERIOD_SIZE * OUT_SHORT_PERIOD_COUNT,
    // .start_threshold = 0,
    // .stop_threshold = 0,
    // .silence_threshold = 0,
    // .avail_min = 0,
};

struct pcm_config pcm_config_in = {
    .channels = 2,
    .rate = IN_SAMPLING_RATE,
    .period_size = IN_PERIOD_SIZE,
    .period_count = IN_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 1,
    .stop_threshold = (IN_PERIOD_SIZE * IN_PERIOD_COUNT),
    // .start_threshold = 0,
    // .stop_threshold = 0,
    // .silence_threshold = 0,
    // .avail_min = 0,
};

struct pcm_config pcm_config_in_low_latency = {
    .channels = 2,
    .rate = IN_SAMPLING_RATE,
    .period_size = IN_PERIOD_SIZE_LOW_LATENCY,
    .period_count = IN_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 1,
    .stop_threshold = (IN_PERIOD_SIZE_LOW_LATENCY * IN_PERIOD_COUNT),
};

struct audio_device {
    struct audio_hw_device hw_device;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    audio_mode_t mode;
    int out_device;
    int in_device;
    int in_source;
    bool standby;
    bool mic_mute;
    // struct audio_route *ar;
    // struct mixer* mixer;
    bool screen_off;

    // RIL
    bool incall_mode;
    float voice_volume;
    tty_modes_t tty_mode;
    bool bt_nrec;

    int lock_cnt;

    struct stream_out *active_out;
    struct stream_in *active_in;
};

struct stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm *pcm;
    struct pcm_config *pcm_config;
    bool standby;
    uint64_t written; /* total frames written, not cleared when entering standby */

    struct resampler_itfe *resampler;
    int16_t *buffer;
    size_t buffer_frames;

    int write_threshold;
    int cur_write_threshold;
    int buffer_type;

    bool sleep_req;
    int lock_cnt;

    struct audio_device *dev;
};

struct stream_in {
    struct audio_stream_in stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm *pcm;
    struct pcm_config *pcm_config;          /* current configuration */
    bool standby;

    unsigned int requested_rate;
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;

    // int16_t *buffer;
    // size_t buffer_size;
    // size_t frames_in;
    int16_t *read_buf;
    size_t read_buf_size;
    size_t read_buf_frames;

    int16_t *proc_buf_in;
    int16_t *proc_buf_out;
    size_t proc_buf_size;
    size_t proc_buf_frames;

    int read_status;

    int num_preprocessors;
    struct effect_info_s preprocessors[MAX_PREPROCESSORS];

    bool sleep_req;
    int lock_cnt;

    struct audio_device *dev;
};


static struct mixer* open_mixer();
static void close_mixer(struct mixer* mixer);

static void select_devices(struct audio_device *adev, struct mixer* mixer);
static void select_input_source(struct audio_device *adev, struct mixer* mixer);
static void select_voice_route(struct audio_device *adev, struct mixer* mixer);

static uint32_t out_get_sample_rate(const struct audio_stream *stream);
static size_t out_get_buffer_size(const struct audio_stream *stream);
static audio_format_t out_get_format(const struct audio_stream *stream);
static uint32_t in_get_sample_rate(const struct audio_stream *stream);
static size_t in_get_buffer_size(const struct audio_stream *stream);
static audio_format_t in_get_format(const struct audio_stream *stream);
static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                                   struct resampler_buffer* buffer);
static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                                  struct resampler_buffer* buffer);

static void out_lock(struct stream_out *out);
static void out_unlock(struct stream_out *out);
static void in_lock(struct stream_in *in);
static void in_unlock(struct stream_in *in);
static void adev_lock(struct audio_device *adev);
static void adev_unlock(struct audio_device *adev);

/* secril-client */
static void*           mSecRilLibHandle;
static HRilClient      mRilClient;
static bool            mActivatedCP;
static HRilClient      (*openClientRILD)  (void);
static int             (*disconnectRILD)  (HRilClient);
static int             (*closeClientRILD) (HRilClient);
static int             (*isConnectedRILD) (HRilClient);
static int             (*connectRILD)     (HRilClient);
static int             (*setCallVolume)   (HRilClient, SoundType, int);
static int             (*setCallAudioPath)(HRilClient, AudioPath);
static int             (*setCallClockSync)(HRilClient, SoundClockCondition);
static void            loadRILD(void);
static status_t        connectRILDIfRequired(void);

/* secril helper functions */

static void loadRILD(void)
{
    mSecRilLibHandle = dlopen("libsecril-client.so", RTLD_NOW);

    if (mSecRilLibHandle) {
        ALOGD("libsecril-client.so is loaded");

        openClientRILD   = (HRilClient (*)(void))
                              dlsym(mSecRilLibHandle, "OpenClient_RILD");
        disconnectRILD   = (int (*)(HRilClient))
                              dlsym(mSecRilLibHandle, "Disconnect_RILD");
        closeClientRILD  = (int (*)(HRilClient))
                              dlsym(mSecRilLibHandle, "CloseClient_RILD");
        isConnectedRILD  = (int (*)(HRilClient))
                              dlsym(mSecRilLibHandle, "isConnected_RILD");
        connectRILD      = (int (*)(HRilClient))
                              dlsym(mSecRilLibHandle, "Connect_RILD");
        setCallVolume    = (int (*)(HRilClient, SoundType, int))
                              dlsym(mSecRilLibHandle, "SetCallVolume");
        setCallAudioPath = (int (*)(HRilClient, AudioPath))
                              dlsym(mSecRilLibHandle, "SetCallAudioPath");
        setCallClockSync = (int (*)(HRilClient, SoundClockCondition))
                              dlsym(mSecRilLibHandle, "SetCallClockSync");

        if (!openClientRILD  || !disconnectRILD   || !closeClientRILD ||
            !isConnectedRILD || !connectRILD      ||
            !setCallVolume   || !setCallAudioPath || !setCallClockSync) {
            ALOGE("Can't load all functions from libsecril-client.so");

            dlclose(mSecRilLibHandle);
            mSecRilLibHandle = NULL;
        } else {
            mRilClient = openClientRILD();
            if (!mRilClient) {
                ALOGE("OpenClient_RILD() error");

                dlclose(mSecRilLibHandle);
                mSecRilLibHandle = NULL;
            } else {
                ALOGE("OpenClient_RILD() done");
            }
        }
    } else {
        ALOGE("Can't load libsecril-client.so");
    }
}

static status_t connectRILDIfRequired(void)
{
    if (!mSecRilLibHandle) {
        ALOGE("connectIfRequired() lib is not loaded");
        return INVALID_OPERATION;
    }

    if (isConnectedRILD(mRilClient)) {
        return OK;
    }

    if (connectRILD(mRilClient) != RIL_CLIENT_ERR_SUCCESS) {
        ALOGE("Connect_RILD() error");
        return INVALID_OPERATION;
    }

    return OK;
}

static int set_voice_volume(struct audio_device *adev, float volume)
{
    ALOGD("### setVoiceVolume_l");

    int mode = adev->mode;
    adev->voice_volume = volume;

    adev_lock(adev);

    if ( (mode == AUDIO_MODE_IN_CALL) && (mSecRilLibHandle) &&
         (connectRILDIfRequired() == OK) ) {

        // uint32_t device = AUDIO_DEVICE_OUT_EARPIECE;
        // if (mOutput != 0) {
        //     device = adev->out_device;
        // }

        uint32_t device = adev->out_device;
        int int_volume = (int)(volume * 5);
        SoundType type;

        ALOGD("### route(%d) call volume(%f)", device, volume);
        switch (device) {
            case AUDIO_DEVICE_OUT_EARPIECE:
                ALOGD("### earpiece call volume");
                type = SOUND_TYPE_VOICE;
                break;

            case AUDIO_DEVICE_OUT_SPEAKER:
            case AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET:
                ALOGD("### speaker call volume");
                type = SOUND_TYPE_SPEAKER;
                break;

            case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
            case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
            case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
                ALOGD("### bluetooth call volume");
                type = SOUND_TYPE_BTVOICE;
                break;

            case AUDIO_DEVICE_OUT_WIRED_HEADSET:
            case AUDIO_DEVICE_OUT_WIRED_HEADPHONE: // Use receive path with 3 pole headset.
                ALOGD("### headset call volume");
                type = SOUND_TYPE_HEADSET;
                break;

            default:
                ALOGW("### Call volume setting error!!!0x%08x \n", device);
                type = SOUND_TYPE_VOICE;
                break;
        }
        setCallVolume(mRilClient, type, int_volume);
    }

    adev_unlock(adev);

    ALOGD("### setVoiceVolume_l: done");

    return 0;
}

static status_t set_incall_path(struct audio_device* adev)
{
    int out_device = adev->out_device;
    int mode = adev->mode;
    bool bt_nrec = adev->bt_nrec;

    ALOGV("set_incall_path: device %x", out_device);

    // Setup sound path for CP clocking
    if ((mSecRilLibHandle) &&
        (connectRILDIfRequired() == OK)) {

        if (mode == AUDIO_MODE_IN_CALL) {
            ALOGD("### incall mode route (%d)", out_device);
            AudioPath path;

            switch(out_device){
                case AUDIO_DEVICE_OUT_EARPIECE:
                    ALOGD("### incall mode earpiece route");
                    path = SOUND_AUDIO_PATH_HANDSET;
                    break;

                case AUDIO_DEVICE_OUT_SPEAKER:
                case AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET:
                    ALOGD("### incall mode speaker route");
                    path = SOUND_AUDIO_PATH_SPEAKER;
                    break;

                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
                    ALOGD("### incall mode bluetooth route %s NR", bt_nrec ? "" : "NO");
                    if (bt_nrec) {
                        path = SOUND_AUDIO_PATH_BLUETOOTH;
                    } else {
                        path = SOUND_AUDIO_PATH_BLUETOOTH_NO_NR;
                    }
                    break;

                case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
                    ALOGD("### incall mode headphone route");
                    path = SOUND_AUDIO_PATH_HEADPHONE;
                    break;
                case AUDIO_DEVICE_OUT_WIRED_HEADSET:
                    ALOGD("### incall mode headset route");
                    path = SOUND_AUDIO_PATH_HEADSET;
                    break;
                default:
                    ALOGW("### incall mode Error!! route = [%d]", out_device);
                    path = SOUND_AUDIO_PATH_HANDSET;
                    break;
            }

            setCallAudioPath(mRilClient, path);

            // if (mMixer != NULL) {
            //     TRACE_DRIVER_IN(DRV_MIXER_GET)
            //     struct mixer_ctl *ctl= mixer_get_ctl_by_name(mMixer, "Voice Call Path");
            //     TRACE_DRIVER_OUT
            //     ALOGE_IF(ctl == NULL, "setIncallPath_l() could not get mixer ctl");
            //     if (ctl != NULL) {
            //         ALOGV("setIncallPath_l() Voice Call Path, (%x)", device);
            //         TRACE_DRIVER_IN(DRV_MIXER_SEL)
            //         mixer_ctl_set_enum_by_string(ctl, getVoiceRouteFromDevice(device));
            //         TRACE_DRIVER_OUT
            //     }
            // }

            struct mixer* mixer;
            mixer = open_mixer();
            select_voice_route(adev, mixer);
            close_mixer(mixer);
        }
    }
    return NO_ERROR;
}


/*
 * NOTE: when multiple mutexes have to be acquired, always take the
 * audio_device mutex first, followed by the stream_in and/or
 * stream_out mutexes.
 */

/* Helper functions */

static struct mixer* open_mixer()
{
    struct mixer* mixer;

    mixer = mixer_open(0);
    if (mixer == NULL) {
        ALOGE("open_mixer() cannot open mixer");
    }

    return mixer;
}

static void close_mixer(struct mixer* mixer)
{
    mixer_close(mixer);
}

static const char* get_output_route(struct audio_device *adev)
{
    int out_device = adev->out_device;
    unsigned int mode = adev->mode;

    ALOGV("get_output_route() out_device %d mode %u", out_device, mode);
    switch(out_device) {
        case AUDIO_DEVICE_OUT_EARPIECE:
            return "RCV";
        case AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET:
            if (mode == AUDIO_MODE_RINGTONE) return "RING_SPK";
            else return "LINEOUT";
        case AUDIO_DEVICE_OUT_SPEAKER:
            if (mode == AUDIO_MODE_RINGTONE) return "RING_SPK";
            else return "SPK";
        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
            if (mode == AUDIO_MODE_RINGTONE) return "RING_HP";
            else return "HP";
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            if (mode == AUDIO_MODE_RINGTONE) return "RING_NO_MIC";
            else return "HP_NO_MIC";
        case (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_WIRED_HEADSET):
        case (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_WIRED_HEADPHONE):
            if (mode == AUDIO_MODE_RINGTONE) return "RING_SPK_HP";
            else return "SPK_HP";
        case (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET):
            if (mode == AUDIO_MODE_RINGTONE) return "RING_SPK";
            else return "SPK_LINEOUT";
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
            return "BT";
        default:
            return "OFF";
    }
}

static const char* get_input_route(struct audio_device *adev)
{
    int in_device = adev->in_device;
    unsigned int mode = adev->mode;

    if (adev->mic_mute)
        return "MIC OFF";

    ALOGV("get_input_route() in_device %d mode %u", in_device, mode);
    switch(in_device) {
        case AUDIO_DEVICE_IN_BUILTIN_MIC:
            return "Main Mic";
        case AUDIO_DEVICE_IN_WIRED_HEADSET:
            return "Hands Free Mic";
        case AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
            return "BT Sco Mic";
        default:
            return "MIC OFF";
    }
}

static void select_devices(struct audio_device *adev, struct mixer* mixer)
{
    // struct mixer* mixer = adev->mixer;
    struct mixer_ctl *m_route_ctl = NULL;

    m_route_ctl = mixer_get_ctl_by_name(mixer, "Playback Path");
    const char* out_route = get_output_route(adev);

    mixer_ctl_set_enum_by_string(m_route_ctl, out_route);
    ALOGD("select_devices(): Playback Path = %s", out_route);


    m_route_ctl = mixer_get_ctl_by_name(mixer, "Capture MIC Path");
    const char* in_route = get_input_route(adev);

    mixer_ctl_set_enum_by_string(m_route_ctl, in_route);
    ALOGD("select_devices(): Capture MIC Path = %s", in_route);
}

static void select_input_source(struct audio_device *adev, struct mixer* mixer)
{
    // int input_source = adev->in_source;

    // switch (input_source) {
    //     case AUDIO_SOURCE_DEFAULT:
    //     case AUDIO_SOURCE_MIC:
    //     case AUDIO_SOURCE_VOICE_COMMUNICATION:
    //         audio_route_apply_path(adev->ar, "in-source-default");
    //         break;
    //     case AUDIO_SOURCE_CAMCORDER:
    //         audio_route_apply_path(adev->ar, "in-source-camcorder");
    //         break;
    //     case AUDIO_SOURCE_VOICE_RECOGNITION:
    //         audio_route_apply_path(adev->ar, "in-source-voice-recog");
    //         break;
    //     case AUDIO_SOURCE_VOICE_UPLINK:
    //     case AUDIO_SOURCE_VOICE_DOWNLINK:
    //     case AUDIO_SOURCE_VOICE_CALL:
    //     default:
    //         audio_route_apply_path(adev->ar, "in-source-default");
    //         break;
    //  }

    // audio_route_update_mixer(adev->ar);

    const char* source_name;
    int input_source = adev->in_source;
    struct mixer_ctl *ctl= mixer_get_ctl_by_name(mixer, "Input Source");

    if (ctl == NULL) {
        ALOGE("select_input_source: Error: Could not open mixer.");
        return;
    }

    switch (input_source) {
        case AUDIO_SOURCE_DEFAULT:
        case AUDIO_SOURCE_MIC:
        case AUDIO_SOURCE_VOICE_COMMUNICATION:
            source_name = "Default";
            break;
        case AUDIO_SOURCE_CAMCORDER:
            source_name = "Camcorder";
            break;
        case AUDIO_SOURCE_VOICE_RECOGNITION:
            source_name = "Voice Recognition";
            break;
        case AUDIO_SOURCE_VOICE_UPLINK:
        case AUDIO_SOURCE_VOICE_DOWNLINK:
        case AUDIO_SOURCE_VOICE_CALL:
        default:
            source_name = "Default";
            break;
     }

    mixer_ctl_set_enum_by_string(ctl, source_name);
    ALOGD("select_input_source %s", source_name);

    ALOGD("select_input_source: done.");
 }

static void select_voice_route(struct audio_device *adev, struct mixer* mixer)
{

    int out_device = adev->out_device;
    tty_modes_t tty_mode = adev->tty_mode;
    struct mixer_ctl *ctl= mixer_get_ctl_by_name(mixer, "Voice Call Path");

    switch (out_device) {
        case AUDIO_DEVICE_OUT_EARPIECE:
            mixer_ctl_set_enum_by_string(ctl, "RCV");
        case AUDIO_DEVICE_OUT_SPEAKER:
        case AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET:
            mixer_ctl_set_enum_by_string(ctl, "SPK");
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
            switch (tty_mode) {
            case TTY_MODE_VCO:
                mixer_ctl_set_enum_by_string(ctl, "TTY_VCO");
            case TTY_MODE_HCO:
                mixer_ctl_set_enum_by_string(ctl, "TTY_HCO");
            case TTY_MODE_FULL:
                mixer_ctl_set_enum_by_string(ctl, "TTY_FULL");
            case TTY_MODE_OFF:
            default:
                if (out_device == AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
                    mixer_ctl_set_enum_by_string(ctl, "HP_NO_MIC");
                } else {
                    mixer_ctl_set_enum_by_string(ctl, "HP");
                }
            }
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
            mixer_ctl_set_enum_by_string(ctl, "BT");
        default:
            mixer_ctl_set_enum_by_string(ctl, "OFF");
    }

    ALOGD("select_voice_route() out_device %d tty_mode %d", out_device, tty_mode);
}

/* must be called with hw device and output stream mutexes locked */
static void do_out_standby(struct stream_out *out)
{
    struct audio_device *adev = out->dev;

    if (!out->standby) {
        pcm_close(out->pcm);
        out->pcm = NULL;
        adev->active_out = NULL;
        if (out->resampler) {
            release_resampler(out->resampler);
            out->resampler = NULL;
        }
        if (out->buffer) {
            free(out->buffer);
            out->buffer = NULL;
        }
        out->standby = true;
    } else {
        ALOGD("do_out_standby() did nothing. Called with out->standby already true.");
    }
}

/* must be called with hw device and input stream mutexes locked */
static void do_in_standby(struct stream_in *in)
{
    struct audio_device *adev = in->dev;

    if (!in->standby) {
        pcm_close(in->pcm);
        in->pcm = NULL;
        adev->active_in = NULL;
        if (in->resampler) {
            release_resampler(in->resampler);
            in->resampler = NULL;
        }
        if (in->read_buf) {
            free(in->read_buf);
            in->read_buf = NULL;
        }

        if (in->proc_buf_in) {
            free(in->proc_buf_in);
            in->proc_buf_in = NULL;
        }
        if (in->proc_buf_out) {
            free(in->proc_buf_out);
            in->proc_buf_out = NULL;
        }

        in->standby = true;
    } else {
        ALOGD("do_in_standby() did nothing. Called with in->standby already true.");
    }
}

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct stream_out *out)
{
    struct audio_device *adev = out->dev;
    unsigned int device;
    int ret;

    ALOGD("start_output_stream()");

    device = PCM_DEVICE;
    out->pcm_config = &pcm_config_out;
    out->buffer_type = OUT_BUFFER_TYPE_UNKNOWN;

    out->pcm = pcm_open(PCM_CARD, device, PCM_OUT | PCM_NORESTART | PCM_MONOTONIC, out->pcm_config);

    if (out->pcm && !pcm_is_ready(out->pcm)) {
        ALOGE("pcm_open(out) failed: %s", pcm_get_error(out->pcm));
        pcm_close(out->pcm);
        return -ENOMEM;
    }
    ALOGE("pcm_open(out) opened");

    /*
     * If the stream rate differs from the PCM rate, we need to
     * create a resampler.
     */
    if (out_get_sample_rate(&out->stream.common) != out->pcm_config->rate) {
        ret = create_resampler(out_get_sample_rate(&out->stream.common),
                               out->pcm_config->rate,
                               out->pcm_config->channels,
                               RESAMPLER_QUALITY_DEFAULT,
                               NULL,
                               &out->resampler);
        out->buffer_frames = (pcm_config_out.period_size * out->pcm_config->rate) /
                out_get_sample_rate(&out->stream.common) + 1;

        out->buffer = malloc(pcm_frames_to_bytes(out->pcm, out->buffer_frames));

        ALOGE("pcm_open(out) created resampler. %d -> %d", out_get_sample_rate(&out->stream.common),
            out->pcm_config->rate);
    }

    adev->active_out = out;

    ALOGD("start_output_stream() done");

    return 0;
}

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct stream_in *in)
{
    struct audio_device *adev = in->dev;
    int ret;

    ALOGD("start_input_stream()");

    in->pcm = pcm_open(PCM_CARD, PCM_DEVICE, PCM_IN, in->pcm_config);

    if (in->pcm && !pcm_is_ready(in->pcm)) {
        ALOGE("pcm_open(in) failed: %s", pcm_get_error(in->pcm));
        pcm_close(in->pcm);
        return -ENOMEM;
    }
    ALOGD("start_input_stream() opened");

    /*
     * If the stream rate differs from the PCM rate, we need to
     * create a resampler.
     */
    if (in_get_sample_rate(&in->stream.common) != in->pcm_config->rate) {
        in->buf_provider.get_next_buffer = get_next_buffer;
        in->buf_provider.release_buffer = release_buffer;

        ret = create_resampler(in->pcm_config->rate,
                               in_get_sample_rate(&in->stream.common),
                               1,
                               RESAMPLER_QUALITY_DEFAULT,
                               &in->buf_provider,
                               &in->resampler);
        ALOGD("start_input_stream() created resampler %d -> %d", in->pcm_config->rate,
            in_get_sample_rate(&in->stream.common));
    }
    in->read_buf_size = pcm_frames_to_bytes(in->pcm,
                                          in->pcm_config->period_size);
    in->read_buf = malloc(in->read_buf_size);
    in->read_buf_frames = 0;

    adev->active_in = in;

    ALOGD("start_input_stream() done");
    return 0;
}

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                                   struct resampler_buffer* buffer)
{
    struct stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return -EINVAL;

    in = (struct stream_in *)((char *)buffer_provider -
                                   offsetof(struct stream_in, buf_provider));

    if (in->pcm == NULL) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        in->read_status = -ENODEV;
        return -ENODEV;
    }

    if (in->read_buf_frames == 0) {
        in->read_status = pcm_read(in->pcm,
                                   (void*)in->read_buf,
                                   in->read_buf_size);
        if (in->read_status != 0) {
            ALOGE("get_next_buffer() pcm_read error %d", in->read_status);
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return in->read_status;
        }
        in->read_buf_frames = in->pcm_config->period_size;
        if (in->pcm_config->channels == 2) {
            unsigned int i;

            /* Discard right channel */
            for (i = 1; i < in->read_buf_frames; i++)
                in->read_buf[i] = in->read_buf[i * 2];
        }
    }

    buffer->frame_count = (buffer->frame_count > in->read_buf_frames) ?
                                in->read_buf_frames : buffer->frame_count;
    buffer->i16 = in->read_buf + (in->pcm_config->period_size - in->read_buf_frames);

    return in->read_status;

}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                                  struct resampler_buffer* buffer)
{
    struct stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return;

    in = (struct stream_in *)((char *)buffer_provider -
                                   offsetof(struct stream_in, buf_provider));

    in->read_buf_frames -= buffer->frame_count;
}

/* read_frames() reads frames from kernel driver, down samples to capture rate
 * if necessary and output the number of frames requested to the buffer specified */
static ssize_t read_frames(struct stream_in *in, void *buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;

    while (frames_wr < frames) {
        size_t frames_rd = frames - frames_wr;
        if (in->resampler != NULL) {
            in->resampler->resample_from_provider(in->resampler,
                    (int16_t *)((char *)buffer +
                            frames_wr * audio_stream_in_frame_size(&in->stream)),
                    &frames_rd);
        } else {
            struct resampler_buffer buf = {
                    { raw : NULL, },
                    frame_count : frames_rd,
            };
            get_next_buffer(&in->buf_provider, &buf);
            if (buf.raw != NULL) {
                memcpy((char *)buffer +
                           frames_wr * audio_stream_in_frame_size(&in->stream),
                        buf.raw,
                        buf.frame_count * audio_stream_in_frame_size(&in->stream));
                frames_rd = buf.frame_count;
            }
            release_buffer(&in->buf_provider, &buf);
        }
        /* in->read_status is updated by getNextBuffer() also called by
         * in->resampler->resample_from_provider() */
        if (in->read_status != 0)
            return in->read_status;

        frames_wr += frames_rd;
    }
    return frames_wr;
}

/* process_frames() reads frames from kernel driver (via read_frames()),
 * calls the active audio pre processings and output the number of frames requested
 * to the buffer specified */
static ssize_t process_frames(struct stream_in *in, void* buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;
    audio_buffer_t in_buf;
    audio_buffer_t out_buf;
    int i;
    // bool has_aux_channels = (~in->main_channels & in->aux_channels);
    void *proc_buf_out;

    // if (has_aux_channels)
    //     proc_buf_out = in->proc_buf_out;
    // else
        proc_buf_out = buffer;

    /* since all the processing below is done in frames and using the config.channels
     * as the number of channels, no changes is required in case aux_channels are present */
    while (frames_wr < frames) {
        /* first reload enough frames at the end of process input buffer */
        if (in->proc_buf_frames < (size_t)frames) {
            ssize_t frames_rd;

            if (in->proc_buf_size < (size_t)frames) {
                size_t size_in_bytes = pcm_frames_to_bytes(in->pcm, frames);

                in->proc_buf_size = (size_t)frames;
                in->proc_buf_in = (int16_t *)realloc(in->proc_buf_in, size_in_bytes);
                ALOG_ASSERT((in->proc_buf_in != NULL),
                            "process_frames() failed to reallocate proc_buf_in");
                // if (has_aux_channels) {
                //     in->proc_buf_out = (int16_t *)realloc(in->proc_buf_out, size_in_bytes);
                //     ALOG_ASSERT((in->proc_buf_out != NULL),
                //                 "process_frames() failed to reallocate proc_buf_out");
                //     proc_buf_out = in->proc_buf_out;
                // }
                ALOGV("process_frames(): proc_buf_in %p extended to %d bytes",
                     in->proc_buf_in, size_in_bytes);
            }
            frames_rd = read_frames(in,
                                    in->proc_buf_in +
                                        in->proc_buf_frames * in->pcm_config->channels,
                                    frames - in->proc_buf_frames);
            if (frames_rd < 0) {
                frames_wr = frames_rd;
                break;
            }
            in->proc_buf_frames += frames_rd;
        }

        // if (in->echo_reference != NULL)
        //     push_echo_reference(in, in->proc_buf_frames);

         /* in_buf.frameCount and out_buf.frameCount indicate respectively
          * the maximum number of frames to be consumed and produced by process() */
        in_buf.frameCount = in->proc_buf_frames;
        in_buf.s16 = in->proc_buf_in;
        out_buf.frameCount = frames - frames_wr;
        out_buf.s16 = (int16_t *)proc_buf_out + frames_wr * in->pcm_config->channels;

        /* FIXME: this works because of current pre processing library implementation that
         * does the actual process only when the last enabled effect process is called.
         * The generic solution is to have an output buffer for each effect and pass it as
         * input to the next.
         */
        for (i = 0; i < in->num_preprocessors; i++) {
            (*in->preprocessors[i].effect_itfe)->process(in->preprocessors[i].effect_itfe,
                                               &in_buf,
                                               &out_buf);
        }

        /* process() has updated the number of frames consumed and produced in
         * in_buf.frameCount and out_buf.frameCount respectively
         * move remaining frames to the beginning of in->proc_buf_in */
        in->proc_buf_frames -= in_buf.frameCount;

        if (in->proc_buf_frames) {
            memcpy(in->proc_buf_in,
                   in->proc_buf_in + in_buf.frameCount * in->pcm_config->channels,
                   in->proc_buf_frames * in->pcm_config->channels * sizeof(int16_t));
        }

        /* if not enough frames were passed to process(), read more and retry. */
        if (out_buf.frameCount == 0) {
            ALOGW("No frames produced by preproc");
            continue;
        }

        if ((frames_wr + (ssize_t)out_buf.frameCount) <= frames) {
            frames_wr += out_buf.frameCount;
        } else {
            /* The effect does not comply to the API. In theory, we should never end up here! */
            ALOGE("preprocessing produced too many frames: %d + %d  > %d !",
                  (unsigned int)frames_wr, out_buf.frameCount, (unsigned int)frames);
            frames_wr = frames;
        }
    }

    /* Remove aux_channels that have been added on top of main_channels
     * Assumption is made that the channels are interleaved and that the main
     * channels are first. */
    // if (has_aux_channels)
    // {
    //     size_t src_channels = in->pcm_config->channels;
    //     size_t dst_channels = popcount(in->main_channels);
    //     int16_t* src_buffer = (int16_t *)proc_buf_out;
    //     int16_t* dst_buffer = (int16_t *)buffer;

    //     if (dst_channels == 1) {
    //         for (i = frames_wr; i > 0; i--)
    //         {
    //             *dst_buffer++ = *src_buffer;
    //             src_buffer += src_channels;
    //         }
    //     } else {
    //         for (i = frames_wr; i > 0; i--)
    //         {
    //             memcpy(dst_buffer, src_buffer, dst_channels*sizeof(int16_t));
    //             dst_buffer += dst_channels;
    //             src_buffer += src_channels;
    //         }
    //     }
    // }

    return frames_wr;
}

static void out_lock(struct stream_out *out) {
    pthread_mutex_lock(&out->lock);
    out->lock_cnt++;
    ALOGV("out_lock() %d", out->lock_cnt);
}

static void out_unlock(struct stream_out *out) {
    out->lock_cnt--;
    ALOGV("out_unlock() %d", out->lock_cnt);
    pthread_mutex_unlock(&out->lock);
}

static void in_lock(struct stream_in *in) {
    pthread_mutex_lock(&in->lock);
    in->lock_cnt++;
    ALOGV("in_lock() %d", in->lock_cnt);
}

static void in_unlock(struct stream_in *in) {
    in->lock_cnt--;
    ALOGV("in_unlock() %d", in->lock_cnt);
    pthread_mutex_unlock(&in->lock);
}

static void adev_lock(struct audio_device *adev) {
    pthread_mutex_lock(&adev->lock);
    adev->lock_cnt++;
    ALOGV("adev_lock() %d", adev->lock_cnt);
}

static void adev_unlock(struct audio_device *adev) {
    adev->lock_cnt--;
    ALOGV("adev_unlock() %d", adev->lock_cnt);
    pthread_mutex_unlock(&adev->lock);
}


/* API functions */

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    return pcm_config_out.rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    return pcm_config_out.period_size *
               audio_stream_out_frame_size((const struct audio_stream_out *)stream);
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    return -ENOSYS;
}

static int out_standby(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGD("out_standby()");

    out->sleep_req = true;
    out_lock(out);
    out->sleep_req = false;
    adev_lock(out->dev);
    do_out_standby(out);
    adev_unlock(out->dev);
    out_unlock(out);

    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ALOGD("out_dump()");
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    int val;

    ALOGD("out_set_parameters()");

    parms = str_parms_create_str(kvpairs);

    out->sleep_req = true;
    out_lock(out);
    out->sleep_req = false;
    adev_lock(adev);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING,
                            value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if ((adev->out_device != val) && (val != 0)) {

            if (adev->mode != AUDIO_MODE_IN_CALL) {
                if (!out->standby)
                   do_out_standby(out);
            }

            if (adev->mode == AUDIO_MODE_IN_CALL) {
                set_incall_path(adev);
            }

            adev->out_device = (int)val;
            // select_devices(adev);
        }
    }

    adev_unlock(adev);
    out_unlock(out);

    str_parms_destroy(parms);
    return ret;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    size_t period_count;

    period_count = OUT_LONG_PERIOD_COUNT;

    return (pcm_config_out.period_size * period_count * 1000) / pcm_config_out.rate;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    return -ENOSYS;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    int ret = 0;
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    size_t frame_size = audio_stream_out_frame_size(stream);
    int16_t *in_buffer = (int16_t *)buffer;
    size_t in_frames = bytes / frame_size;
    size_t out_frames;
    int buffer_type;
    int kernel_frames;

    bool in_locked = false;
    bool restart_input = false;

     ALOGV("-----out_write(%p, %d) START", buffer, (int)bytes);

    if (out->sleep_req) {
        // 10ms are always shorter than the time to reconfigure the audio path
        // which is the only condition when sleep_req would be true.
        ALOGD("out_write(): out->sleep_req: sleeping");
        usleep(10000);

        if (adev->active_out != out) {
            ALOGE("out_write() active out changed. abandoning this session.");
            // out_standby(out);
            // return 0;
            // out = adev->active_out;
            // return -EPIPE;
        }
    }

    /*
     * acquiring hw device mutex systematically is useful if a low
     * priority thread is waiting on the output stream mutex - e.g.
     * executing out_set_parameters() while holding the hw device
     * mutex
     */
    out_lock(out);
    if (out->standby) {
        ALOGD("out_write(): pcm playback is exiting standby %x.", out);
        adev_lock(adev);

        struct stream_in* in = adev->active_in;
        while (in != NULL && !in->standby) {
            ALOGD("out_write(): Warning: active_in is present.");

            // undo locks so that input can be locked in proper order
            adev_unlock(adev);

            ALOGV("out_write(): take input locks.");
            in->sleep_req = true;
            in_lock(in);
            // in->sleep_req = false;
            adev_lock(adev);

            // if (in == adev->active_in && in->standby == false) {
            if (in == adev->active_in) {
                // Here the input is locked and a sleep has been requested
                if (!in->standby) {
                    ALOGD("out_write(): Warning: active_in is present and NOT in standby.");
                    restart_input = true;
                    ALOGD("out_write(): forcing input standby");
                    do_in_standby(in);
                }

                ALOGD("out_write(): input wait done.");
                in_locked = true;
                break;
            }

            // restore the locks
            ALOGD("out_write(): release in lock.");
            in_unlock(in);
            in = adev->active_in;
        }

        ALOGD("out_write(): starting output stream.");
        ret = start_output_stream(out);
        if (ret != 0) {
            // adev_unlock(adev);
            ALOGE("out_write() Error starting output stream.");
            goto exit;
        }
        ALOGD("out_write(): starting output stream done.");

        if (in) {
            if (restart_input && in->standby) {
                ALOGD("out_write(): start input stream.");
                ret = start_input_stream(in);
                if (ret == 0) {
                    in->standby = false;

                    // ALOGD("out_write(): input standby.");
                    // do_in_standby(in);
                }
            }
            if (in_locked) {
                ALOGD("out_write(): release input lock.");
                in_unlock(in);
            }
            in->sleep_req = false;
        }

        /*
         * mixer must be set when coming out of standby
         */
        ALOGD("out_write(): selecting devices.");
        // audio_route_reset(adev->ar);
        struct mixer* mixer;
        mixer = open_mixer();
        select_devices(adev, mixer);
        close_mixer(mixer);
        // audio_route_update_mixer(adev->ar);

        out->standby = false;
        adev_unlock(adev);
        ALOGD("pcm playback is exiting standby. done.");
    }
    buffer_type = (adev->screen_off && !adev->active_in) ?
            OUT_BUFFER_TYPE_LONG : OUT_BUFFER_TYPE_SHORT;

    /* detect changes in screen ON/OFF state and adapt buffer size
     * if needed. Do not change buffer size when routed to SCO device. */
    if (buffer_type != out->buffer_type) {
        size_t period_count;

        if (buffer_type == OUT_BUFFER_TYPE_LONG)
            period_count = OUT_LONG_PERIOD_COUNT;
        else
            period_count = OUT_SHORT_PERIOD_COUNT;

        out->write_threshold = out->pcm_config->period_size * period_count;
        /* reset current threshold if exiting standby */
        if (out->buffer_type == OUT_BUFFER_TYPE_UNKNOWN)
            out->cur_write_threshold = out->write_threshold;
        out->buffer_type = buffer_type;
    }

    /* Reduce number of channels, if necessary */
    if (audio_channel_count_from_out_mask(out_get_channels(&stream->common)) >
                 (int)out->pcm_config->channels) {
        unsigned int i;

        /* Discard right channel */
        for (i = 1; i < in_frames; i++)
            in_buffer[i] = in_buffer[i * 2];

        /* The frame size is now half */
        frame_size /= 2;
    }

    /* Change sample rate, if necessary */
    if (out_get_sample_rate(&stream->common) != out->pcm_config->rate) {
        out_frames = out->buffer_frames;
        out->resampler->resample_from_input(out->resampler,
                                            in_buffer, &in_frames,
                                            out->buffer, &out_frames);
        in_buffer = out->buffer;
    } else {
        out_frames = in_frames;
    }

    {
        int total_sleep_time_us = 0;
        size_t period_size = out->pcm_config->period_size;

        /* do not allow more than out->cur_write_threshold frames in kernel
         * pcm driver buffer */
        do {
            struct timespec time_stamp;
            if (pcm_get_htimestamp(out->pcm,
                                   (unsigned int *)&kernel_frames,
                                   &time_stamp) < 0)
                break;
            kernel_frames = pcm_get_buffer_size(out->pcm) - kernel_frames;

            if (kernel_frames > out->cur_write_threshold) {
                int sleep_time_us =
                    (int)(((int64_t)(kernel_frames - out->cur_write_threshold)
                                    * 1000000) / out->pcm_config->rate);
                if (sleep_time_us < MIN_WRITE_SLEEP_US)
                    break;
                total_sleep_time_us += sleep_time_us;
                if (total_sleep_time_us > MAX_WRITE_SLEEP_US) {
                    ALOGW("out_write() limiting sleep time %d to %d",
                          total_sleep_time_us, MAX_WRITE_SLEEP_US);
                    sleep_time_us = MAX_WRITE_SLEEP_US -
                                        (total_sleep_time_us - sleep_time_us);
                }
                usleep(sleep_time_us);
            }

        } while ((kernel_frames > out->cur_write_threshold) &&
                (total_sleep_time_us <= MAX_WRITE_SLEEP_US));

        /* do not allow abrupt changes on buffer size. Increasing/decreasing
         * the threshold by steps of 1/4th of the buffer size keeps the write
         * time within a reasonable range during transitions.
         * Also reset current threshold just above current filling status when
         * kernel buffer is really depleted to allow for smooth catching up with
         * target threshold.
         */
        if (out->cur_write_threshold > out->write_threshold) {
            out->cur_write_threshold -= period_size / 4;
            if (out->cur_write_threshold < out->write_threshold) {
                out->cur_write_threshold = out->write_threshold;
            }
        } else if (out->cur_write_threshold < out->write_threshold) {
            out->cur_write_threshold += period_size / 4;
            if (out->cur_write_threshold > out->write_threshold) {
                out->cur_write_threshold = out->write_threshold;
            }
        } else if ((kernel_frames < out->write_threshold) &&
            ((out->write_threshold - kernel_frames) >
                (int)(period_size * OUT_SHORT_PERIOD_COUNT))) {
            out->cur_write_threshold = (kernel_frames / period_size + 1) * period_size;
            out->cur_write_threshold += period_size / 4;
        }
    }

    ret = pcm_write(out->pcm, in_buffer, out_frames * frame_size);
    if (ret == -EPIPE) {
        /* In case of underrun, don't sleep since we want to catch up asap */
        // adev_unlock(adev);
        out_unlock(out);

        ALOGV("-----out_write(%p, %d) END WITH ERROR -EPIPE", buffer, (int)bytes);

        return ret;
    }
    if (ret == 0) {
        out->written += out_frames;
    }

exit:
    // adev_unlock(adev);
    out_unlock(out);

    if (ret != 0) {
        usleep(bytes * 1000000 / audio_stream_out_frame_size(&stream->common) /
               out_get_sample_rate(&stream->common));
    }

    ALOGV("-----out_write(%p, %d) END", buffer, (int)bytes);

    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    int status;
    effect_descriptor_t desc;

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        goto exit;

    ALOGD("out_add_audio_effect(), effect type: %08x", desc.type.timeLow);

exit:
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    return -EINVAL;
}

static int out_get_presentation_position(const struct audio_stream_out *stream,
                                   uint64_t *frames, struct timespec *timestamp)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -1;

    out->sleep_req = true;
    out_lock(out);
    out->sleep_req = false;

    if (out->standby) {
        ALOGE("out_get_presentation_position() out stream is in standby.");
        out_unlock(out);
        return -ENOSYS;
    }

    if (out->pcm == NULL) {
        ALOGE("out_get_presentation_position() out->pcm is NULL");
        out_unlock(out);
        return -ENOSYS;
    }

    size_t avail;
    if (pcm_get_htimestamp(out->pcm, &avail, timestamp) == 0) {
        size_t kernel_buffer_size = out->pcm_config->period_size * out->pcm_config->period_count;
        // FIXME This calculation is incorrect if there is buffering after app processor
        int64_t signed_frames = out->written - kernel_buffer_size + avail;
        // It would be unusual for this value to be negative, but check just in case ...
        if (signed_frames >= 0) {
            *frames = signed_frames;
            ret = 0;
        }
    }

    out_unlock(out);

    return ret;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    return in->requested_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGD("in_set_sample_rate()");
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    size_t size;

    /*
     * take resampling into account and return the closest majoring
     * multiple of 16 frames, as audioflinger expects audio buffers to
     * be a multiple of 16 frames
     */
    size = (in->pcm_config->period_size * in_get_sample_rate(stream)) /
            in->pcm_config->rate;
    size = ((size + 15) / 16) * 16;

    return size * audio_stream_in_frame_size(&in->stream);
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    return AUDIO_CHANNEL_IN_MONO;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    return -ENOSYS;
}

static int in_standby(struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("in_standby()");

    in->sleep_req = true;
    in_lock(in);
    in->sleep_req = false;
    adev_lock(in->dev);
    do_in_standby(in);
    adev_unlock(in->dev);
    in_unlock(in);

    ALOGD("in_standby() done");

    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    int val;

    ALOGD("in_set_parameters()");

    parms = str_parms_create_str(kvpairs);

    in->sleep_req = true;
    in_lock(in);
    in->sleep_req = false;
    adev_lock(adev);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE,
                            value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if (adev->in_source != val) {
            adev->in_source = val;

            struct mixer* mixer;
            mixer = open_mixer();
            select_input_source(adev, mixer);
            close_mixer(mixer);
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING,
                            value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value) & ~AUDIO_DEVICE_BIT_IN;
        if ((adev->in_device != val) && (val != 0)) {

            if (adev->mode != AUDIO_MODE_IN_CALL) {
                if (!in->standby)
                    do_in_standby(in);
            }

            adev->in_device = val;
            // select_devices(adev);
        }
    }
    adev_unlock(adev);
    in_unlock(in);

    ALOGD("in_set_parameters() done");

    str_parms_destroy(parms);
    return ret;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    int ret = 0;
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    size_t frames_rq = bytes / audio_stream_in_frame_size(stream);

    bool out_locked = false;

    if (in->sleep_req) {
        // 10ms are always shorter than the time to reconfigure the audio path
        // which is the only condition when sleep_req would be true.
        ALOGD("in->sleep_req: sleeping");
        usleep(10000);

        if (adev->active_in != in) {
            ALOGE("in_read() active in changed. abandoning this session.");
        //     // out = adev->active_out;
        //     return -EPIPE;
        }
    }

    /*
     * acquiring hw device mutex systematically is useful if a low
     * priority thread is waiting on the input stream mutex - e.g.
     * executing in_set_parameters() while holding the hw device
     * mutex
     */
    in_lock(in);
    if (in->standby) {
        ALOGD("in_read() pcm capture is exiting standby.");
        adev_lock(adev);

        struct stream_out* out = adev->active_out;
        while (out && !out->standby) {
            ALOGD("in_read() Warning: active_out is present.");

            // undo locks so we can lock the output in proper order
            adev_unlock(adev);
            in_unlock(in);

            ALOGD("in_read(): initial release locks.");
            // lock output for standby
            out->sleep_req = true;
            out_lock(out);
            // out->sleep_req = false;
            in_lock(in);
            adev_lock(adev);
            ALOGD("in_read(): locks taken.");

            if (out == adev->active_out) {
                out_locked = true;
                break;
            }

            ALOGD("in_read(): release out lock again.");
            out_unlock(out);
            out = adev->active_out;
            ALOGD("in_read(): release out locks again done.");
        }

        if (out && !out->standby) {
            ALOGD("in_read(): output go into standby.");
            do_out_standby(out);

            ALOGD("in_read(): output starting stream.");
            ret = start_output_stream(out);
            if (ret != 0)
                ALOGE("in_read(): Error restarting output stream.");
            out->standby = false;

            // ALOGD("in_read(): output go into standby again.");
            // do_out_standby(out);

            if (out_locked) {
                out_unlock(out);
                out->sleep_req = false;
                ALOGD("in_read(): release output lock.");
            }
            ALOGD("in_read(): restart output done. standby %d.", out->standby);
        }

        ALOGD("in_read(): starting input stream.");
        ret = start_input_stream(in);
        if (ret == 0)
            in->standby = false;

        /*
         * mixer must be set when coming out of standby
         */
        // audio_route_reset(adev->ar);
        struct mixer* mixer;
        mixer = open_mixer();
        select_devices(adev, mixer);
        select_input_source(adev, mixer);
        close_mixer(mixer);
        // audio_route_update_mixer(adev->ar);

        adev_unlock(adev);
        ALOGD("in_read() pcm capture is exiting standby. done.");
    }

    if (ret < 0)
        goto exit;

    /*if (in->num_preprocessors != 0) {
        ret = process_frames(in, buffer, frames_rq);
    } else */if (in->resampler != NULL) {
        ret = read_frames(in, buffer, frames_rq);
    } else if (in->pcm_config->channels == 2) {
        /*
         * If the PCM is stereo, capture twice as many frames and
         * discard the right channel.
         */
        unsigned int i;
        int16_t *in_buffer = (int16_t *)buffer;

        ret = pcm_read(in->pcm, in->read_buf, bytes * 2);

        /* Discard right channel */
        for (i = 0; i < frames_rq; i++)
            in_buffer[i] = in->read_buf[i * 2];
    } else {
        ret = pcm_read(in->pcm, buffer, bytes);
    }

    if (ret > 0)
        ret = 0;

    /*
     * Instead of writing zeroes here, we could trust the hardware
     * to always provide zeroes when muted.
     */
    if (ret == 0 && adev->mic_mute)
        memset(buffer, 0, bytes);

exit:
    if (ret < 0)
        usleep(bytes * 1000000 / audio_stream_in_frame_size(stream) /
               in_get_sample_rate(&stream->common));

    // adev_unlock(adev);
    in_unlock(in);

    ALOGV("in_read() done");

    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    return 0;
}

/* in audio effect helpers */

#define GET_COMMAND_STATUS(status, fct_status, cmd_status) \
            do {                                           \
                if (fct_status != 0)                       \
                    status = fct_status;                   \
                else if (cmd_status != 0)                  \
                    status = cmd_status;                   \
            } while(0)

static int in_configure_reverse(struct stream_in *in)
{
    int32_t cmd_status;
    uint32_t size = sizeof(int);
    effect_config_t config;
    int32_t status = 0;
    int32_t fct_status = 0;
    int i;

    if (in->num_preprocessors > 0) {
        // config.inputCfg.channels = in->main_channels;
        // config.outputCfg.channels = in->main_channels;
        config.inputCfg.channels = in->pcm_config->channels;
        config.outputCfg.channels = in->pcm_config->channels;
        config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
        config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
        config.inputCfg.samplingRate = in->requested_rate;
        config.outputCfg.samplingRate = in->requested_rate;
        config.inputCfg.mask =
                ( EFFECT_CONFIG_SMP_RATE | EFFECT_CONFIG_CHANNELS | EFFECT_CONFIG_FORMAT );
        config.outputCfg.mask =
                ( EFFECT_CONFIG_SMP_RATE | EFFECT_CONFIG_CHANNELS | EFFECT_CONFIG_FORMAT );

        for (i = 0; i < in->num_preprocessors; i++)
        {
            if ((*in->preprocessors[i].effect_itfe)->process_reverse == NULL)
                continue;
            fct_status = (*(in->preprocessors[i].effect_itfe))->command(
                                                        in->preprocessors[i].effect_itfe,
                                                        EFFECT_CMD_SET_CONFIG_REVERSE,
                                                        sizeof(effect_config_t),
                                                        &config,
                                                        &size,
                                                        &cmd_status);
            GET_COMMAND_STATUS(status, fct_status, cmd_status);
        }
    }
    return status;
}

#define MAX_NUM_CHANNEL_CONFIGS 10

static void in_read_audio_effect_channel_configs(struct stream_in *in,
                                                 struct effect_info_s *effect_info)
{
    /* size and format of the cmd are defined in hardware/audio_effect.h */
    effect_handle_t effect = effect_info->effect_itfe;
    uint32_t cmd_size = 2 * sizeof(uint32_t);
    uint32_t cmd[] = { EFFECT_FEATURE_AUX_CHANNELS, MAX_NUM_CHANNEL_CONFIGS };
    /* reply = status + number of configs (n) + n x channel_config_t */
    uint32_t reply_size =
            2 * sizeof(uint32_t) + (MAX_NUM_CHANNEL_CONFIGS * sizeof(channel_config_t));
    int32_t reply[reply_size];
    int32_t cmd_status;

    ALOG_ASSERT((effect_info->num_channel_configs == 0),
                "in_read_audio_effect_channel_configs() num_channel_configs not cleared");
    ALOG_ASSERT((effect_info->channel_configs == NULL),
                "in_read_audio_effect_channel_configs() channel_configs not cleared");

    /* if this command is not supported, then the effect is supposed to return -EINVAL.
     * This error will be interpreted as if the effect supports the main_channels but does not
     * support any aux_channels */
    cmd_status = (*effect)->command(effect,
                                EFFECT_CMD_GET_FEATURE_SUPPORTED_CONFIGS,
                                cmd_size,
                                (void*)&cmd,
                                &reply_size,
                                (void*)&reply);

    if (cmd_status != 0) {
        ALOGV("in_read_audio_effect_channel_configs(): "
                "fx->command returned %d", cmd_status);
        return;
    }

    if (reply[0] != 0) {
        ALOGW("in_read_audio_effect_channel_configs(): "
                "command EFFECT_CMD_GET_FEATURE_SUPPORTED_CONFIGS error %d num configs %d",
                reply[0], (reply[0] == -ENOMEM) ? reply[1] : MAX_NUM_CHANNEL_CONFIGS);
        return;
    }

    /* the feature is not supported */
    ALOGV("in_read_audio_effect_channel_configs()(): "
            "Feature supported and adding %d channel configs to the list", reply[1]);
    effect_info->num_channel_configs = reply[1];
    effect_info->channel_configs =
            (channel_config_t *) malloc(sizeof(channel_config_t) * reply[1]); /* n x configs */
    memcpy(effect_info->channel_configs, (reply + 2), sizeof(channel_config_t) * reply[1]);
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    struct stream_in *in = (struct stream_in *)stream;
    int status;
    effect_descriptor_t desc;

    // pthread_mutex_lock(&in->dev->lock);
    // pthread_mutex_lock(&in->lock);
    in->sleep_req = true;
    in_lock(in);
    in->sleep_req = false;
    adev_lock(in->dev);

    if (in->num_preprocessors >= MAX_PREPROCESSORS) {
        status = -ENOSYS;
        goto exit;
    }

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        goto exit;

    in->preprocessors[in->num_preprocessors].effect_itfe = effect;
    /* add the supported channel of the effect in the channel_configs */
    in_read_audio_effect_channel_configs(in, &in->preprocessors[in->num_preprocessors]);

    in->num_preprocessors++;

    /* check compatibility between main channel supported and possible auxiliary channels */
    // in_update_aux_channels(in, effect);

    ALOGD("in_add_audio_effect(), effect type: %08x", desc.type.timeLow);

    if (memcmp(&desc.type, FX_IID_AEC, sizeof(effect_uuid_t)) == 0) {
        // in->need_echo_reference = true;
        do_in_standby(in);
        in_configure_reverse(in);
    }

exit:

    ALOGW_IF(status != 0, "in_add_audio_effect() error %d", status);
    // pthread_mutex_unlock(&in->lock);
    // pthread_mutex_unlock(&in->dev->lock);
    adev_unlock(in->dev);
    in_unlock(in);
    return status;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    struct stream_in *in = (struct stream_in *)stream;
    int i;
    int status = -EINVAL;
    effect_descriptor_t desc;

    // pthread_mutex_lock(&in->dev->lock);
    // pthread_mutex_lock(&in->lock);
    in->sleep_req = true;
    in_lock(in);
    in->sleep_req = false;
    adev_lock(in->dev);

    if (in->num_preprocessors <= 0) {
        status = -ENOSYS;
        goto exit;
    }

    for (i = 0; i < in->num_preprocessors; i++) {
        if (status == 0) { /* status == 0 means an effect was removed from a previous slot */
            in->preprocessors[i - 1].effect_itfe = in->preprocessors[i].effect_itfe;
            in->preprocessors[i - 1].channel_configs = in->preprocessors[i].channel_configs;
            in->preprocessors[i - 1].num_channel_configs = in->preprocessors[i].num_channel_configs;
            ALOGV("in_remove_audio_effect moving fx from %d to %d", i, i - 1);
            continue;
        }
        if (in->preprocessors[i].effect_itfe == effect) {
            ALOGV("in_remove_audio_effect found fx at index %d", i);
            free(in->preprocessors[i].channel_configs);
            status = 0;
        }
    }

    if (status != 0)
        goto exit;

    in->num_preprocessors--;
    /* if we remove one effect, at least the last preproc should be reset */
    in->preprocessors[in->num_preprocessors].num_channel_configs = 0;
    in->preprocessors[in->num_preprocessors].effect_itfe = NULL;
    in->preprocessors[in->num_preprocessors].channel_configs = NULL;


    /* check compatibility between main channel supported and possible auxiliary channels */
    // in_update_aux_channels(in, NULL);

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        goto exit;

    ALOGD("in_remove_audio_effect(), effect type: %08x", desc.type.timeLow);

    if (memcmp(&desc.type, FX_IID_AEC, sizeof(effect_uuid_t)) == 0) {
        // in->need_echo_reference = false;
        do_in_standby(in);
    }

exit:

    ALOGW_IF(status != 0, "in_remove_audio_effect() error %d", status);
    adev_unlock(in->dev);
    in_unlock(in);
    return status;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out;
    int ret;

    ALOGD("adev_open_output_stream()");

    out = (struct stream_out *)calloc(1, sizeof(struct stream_out));
    if (!out)
        return -ENOMEM;

    if (config->channel_mask != AUDIO_CHANNEL_OUT_STEREO || config->sample_rate != 44100) {
        config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
        config->sample_rate = 44100;
        ALOGE("adev_open_output_stream(): Error invalid channel mask. Requesting stereo output.");
        return -EINVAL;
    }

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    out->stream.get_presentation_position = out_get_presentation_position;

    out->dev = adev;

    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    out->standby = true;
    /* out->written = 0; by calloc() */

    *stream_out = &out->stream;

    // adev_lock(adev);
    // select_devices(adev);
    // // start_output_stream(*out);
    // adev_unlock(adev);

    ALOGD("adev_open_output_stream: done");

    return 0;

err_open:
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    ALOGD("adev_close_output_stream()");

    out_standby(&stream->common);
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret;

    ALOGD("adev_set_parameters()");

    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->screen_off = false;
        else
            adev->screen_off = true;
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_NREC, value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            adev->bt_nrec = true;
        } else {
            adev->bt_nrec = false;
            ALOGD("adev_set_parameters() turning noise reduction and echo cancellation off for BT "
                 "headset");
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_TTY_MODE, value, sizeof(value));
    if (ret >= 0) {
        unsigned int tty_mode;

        if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_OFF) == 0) {
            tty_mode = TTY_MODE_OFF;
        } else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_VCO) == 0) {
            tty_mode = TTY_MODE_VCO;
        } else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_HCO) == 0) {
            tty_mode = TTY_MODE_HCO;
        } else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_FULL) == 0) {
            tty_mode = TTY_MODE_FULL;
        } else {
            tty_mode = TTY_MODE_OFF;
        }

        if (tty_mode != adev->tty_mode) {
            ALOGV("adev_set_parameters() new tty mode %d", tty_mode);
            adev->tty_mode = tty_mode;

            if (adev->out_device && adev->mode == AUDIO_MODE_IN_CALL) {
                // setIncallPath_l(mOutput->device());
                set_incall_path(adev);
            }
        }
    }

    str_parms_destroy(parms);
    return ret;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    return NULL;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    // return -ENOSYS;

    ALOGV("Set master volume to %f.\n", volume);
    // We return an error code here to let the audioflinger do in-software
    // volume on top of the maximum volume that we set through the SND API.
    // return error - software mixer will handle it

    // This is from Crespo device source which is AOSP 4.3.1 based.
    // I don't know if it works for AOSP 5.0

    return -1;
}

#ifndef ICS_AUDIO_BLOB
static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    return -ENOSYS;
}

#endif

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    ALOGD("adev_set_mode()");

    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out = adev->active_out;
    struct stream_in *in = adev->active_in;

    bool out_locked = false;
    bool in_locked = false;

    if (out != NULL && !out->standby) {
        out->sleep_req = true;
        out_lock(out);
        out->sleep_req = false;
        out_locked = true;
    }
    if (in != NULL && !in->standby) {
        in->sleep_req = true;
        in_lock(in);
        in->sleep_req = false;
        in_locked = true;
    }
    adev_lock(adev);

    audio_mode_t prev_mode = adev->mode;
    adev->mode = mode;
    ALOGD("adev_set_mode() : new %d, old %d", mode, prev_mode);


    bool modeNeedsCPActive = mode == AUDIO_MODE_IN_CALL ||
                                mode == AUDIO_MODE_RINGTONE;
    // activate call clock in radio when entering in call or ringtone mode
    if (modeNeedsCPActive)
    {
        if ((!mActivatedCP) && (mSecRilLibHandle) && (connectRILDIfRequired() == OK)) {
            setCallClockSync(mRilClient, SOUND_CLOCK_START);
            mActivatedCP = true;
        }
    }

    if (mode == AUDIO_MODE_IN_CALL && !adev->incall_mode) {
        if (out && !out->standby) {
            ALOGV("adev_set_mode() in call force output standby");
            out_standby(&out->stream.common);
        }
        if (in && !in->standby) {
            ALOGV("adev_set_mode() in call force input standby");
            in_standby(&in->stream.common);
        }

        ALOGV("adev_set_mode() openPcmOut_l()");
        // openPcmOut_l();
        // openMixer_l();
        // setInputSource_l(AUDIO_SOURCE_DEFAULT);
        // setVoiceVolume_l(mVoiceVol);

        start_output_stream(out);

        adev->in_source = AUDIO_SOURCE_DEFAULT;
        struct mixer* mixer;
        mixer = open_mixer();
        select_input_source(adev, mixer);
        close_mixer(mixer);

        set_voice_volume(adev, adev->voice_volume);

        adev->incall_mode = true;
    }

    if (mode != AUDIO_MODE_IN_CALL && adev->incall_mode) {
        
        // if (mMixer != NULL) {
        //     TRACE_DRIVER_IN(DRV_MIXER_GET)
        //     struct mixer_ctl *ctl= mixer_get_ctl_by_name(mMixer, "Playback Path");
        //     TRACE_DRIVER_OUT
        //     if (ctl != NULL) {
        //         ALOGV("setMode() reset Playback Path to RCV");
        //         TRACE_DRIVER_IN(DRV_MIXER_SEL)
        //         mixer_ctl_set_enum_by_string(ctl, "RCV");
        //         TRACE_DRIVER_OUT
        //     }
        // }

        adev->out_device = AUDIO_DEVICE_OUT_EARPIECE;
        struct mixer* mixer;
        mixer = open_mixer();
        select_devices(adev, mixer);
        select_input_source(adev, mixer);
        close_mixer(mixer);

        // ALOGV("setMode() closePcmOut_l()");
        // closeMixer_l();
        // closePcmOut_l();

        if (out && !out->standby) {
            ALOGV("adev_set_mode() in call force output standby");
            out_standby(&out->stream.common);
        }
        if (in && !in->standby) {
            ALOGV("adev_set_mode() in call force input standby");
            in_standby(&in->stream.common);
        }

        adev->incall_mode = false;
    }

    if (!modeNeedsCPActive) {
        if(mActivatedCP)
            mActivatedCP = false;
    }


    adev_unlock(adev);
    if (in != NULL && in_locked)
        in_unlock(in);
    if (out != NULL && out_locked)
        out_unlock(out);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in = adev->active_in;

    ALOGV("adev_set_mic_mute(%d) adev->mic_mute %d", state, adev->mic_mute);

    if (in != NULL) {
        in->sleep_req = true;
        in_lock(in);
        in->sleep_req = false;
        adev_lock(adev);

        // in call mute is handled by RIL
        if (adev->mode != AUDIO_MODE_IN_CALL) {
            do_in_standby(in);
        }

        adev_unlock(adev);
        in_unlock(in);
    }

    adev->mic_mute = state;

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct audio_device *adev = (struct audio_device *)dev;

    *state = adev->mic_mute;

    return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    size_t size;

    /*
     * take resampling into account and return the closest majoring
     * multiple of 16 frames, as audioflinger expects audio buffers to
     * be a multiple of 16 frames
     */
    size = (pcm_config_in.period_size * config->sample_rate) / pcm_config_in.rate;
    size = ((size + 15) / 16) * 16;

    return (size * audio_channel_count_from_in_mask(config->channel_mask) *
                audio_bytes_per_sample(config->format));
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags __unused,
                                  const char *address __unused,
                                  audio_source_t source __unused)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in;
    int ret;

    ALOGD("adev_open_input_stream()");

    *stream_in = NULL;

    /* Respond with a request for mono if a different format is given. */
    if (config->channel_mask != AUDIO_CHANNEL_IN_MONO) {
        config->channel_mask = AUDIO_CHANNEL_IN_MONO;
        ALOGE("adev_open_input_stream(): Error invalid channel mask. Requesting mono input.");
        return -EINVAL;
    }

    in = (struct stream_in *)calloc(1, sizeof(struct stream_in));
    if (!in) {
        ALOGE("adev_open_input_stream(): Error creating ENOMEM");
        return -ENOMEM;
    }

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    in->dev = adev;
    in->standby = true;
    in->requested_rate = config->sample_rate;
    /* default PCM config */
    in->pcm_config = (config->sample_rate == IN_SAMPLING_RATE) && (flags & AUDIO_INPUT_FLAG_FAST) ?
            &pcm_config_in_low_latency : &pcm_config_in;

    ALOGD("adev_open_input_stream() done");

    *stream_in = &in->stream;
    return 0;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *stream)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("adev_close_input_stream()");

    in_standby(&stream->common);

    adev_lock(adev);
    free(stream);
    ALOGD("adev_close_input_stream() done %x", adev->active_in);
    adev->active_in = NULL;
    adev_unlock(adev);
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    ALOGD("adev_dump()");
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct audio_device *adev = (struct audio_device *)device;

    ALOGD("adev_close()");

    // audio_route_free(adev->ar);
    // close_mixer(adev->mixer);

    free(device);
    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct audio_device *adev;
    int ret;

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct audio_device));
    if (!adev)
        return -ENOMEM;

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;

    // adev->ar = audio_route_init(MIXER_CARD, NULL);
    // adev->mixer = open_mixer();
    adev->out_device = AUDIO_DEVICE_NONE;
    // adev->in_device = AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN;
    adev->in_device = AUDIO_DEVICE_NONE;
    adev->in_source = AUDIO_DEVICE_NONE;
    adev->mode = AUDIO_MODE_NORMAL;

    *device = &adev->hw_device.common;

    /* RIL */
    loadRILD();
    adev->voice_volume = 1.0f;

    ALOGD("adev_open: done");

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "GT-P75xx audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
