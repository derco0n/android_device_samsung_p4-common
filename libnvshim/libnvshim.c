/*
 * Copyright (C) 2015 The Android Open Source Project
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

#define LOG_TAG "libnvshim"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <utils/Log.h>

//various funcs we'll need to call, in their mangled form

    // // android::String8::String8(void)
    extern void _ZN7android7String8C1Ev(void **str8P);

    // // android::String8::~String8()
    extern void _ZN7android7String8D1Ev(void **str8P);

    // // android::String8::append(char const*)
    extern void _ZN7android7String86appendEPKc(void **str8P, const char *other);

    // android::SurfaceControl::setLayer(unsigned int)
    extern int _ZN7android14SurfaceControl8setLayerEj(void *surfaceControl, uint32_t layer);

    // android::SurfaceControl::setPosition(float x, float y)
    extern int _ZN7android14SurfaceControl11setPositionEff(void *surfaceControl, float x, float y);

    // // android::SurfaceComposerClient::getDisplayInfo(android::sp<android::IBinder> const&, android::DisplayInfo *)
    extern int _ZN7android21SurfaceComposerClient14getDisplayInfoERKNS_2spINS_7IBinderEEEPNS_11DisplayInfoE(
        void **display,
        void *info);

    // android::SurfaceComposerClient::getBuiltInDisplay(android::SurfaceComposerClient *__hidden this, int)
    extern void* _ZN7android21SurfaceComposerClient17getBuiltInDisplayEi(void *surfaceComposerClient, int32_t id);

    // android::sp<android::IBinder>::~sp()
    extern int _ZN7android2spINS_7IBinderEED2Ev(void *);

    // sp<SurfaceControl> android::SurfaceComposerClient::createSurface(android::String8 const&,uint,uint,int,uint)
    extern void* _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8Ejjij(
        void *surfaceComposerClient,
        void **name,
        uint32_t w,
        uint32_t h,
        int32_t format,
        uint32_t flags);

    // android::Singleton<android::Composer>::getInstance(void)
    extern void* _ZN7android9SingletonINS_8ComposerEE11getInstanceEv(void *this);


    // android::ComposerService::getComposerService(android::ComposerService *this, int a2, int a3)
    extern void* _ZN7android15ComposerService18getComposerServiceEv(void *this);

    // android::Composer::getDisplayStateLocked(android::sp<android::IBinder> const&)
    extern int _ZN7android8Composer21getDisplayStateLockedERKNS_2spINS_7IBinderEEE(void *this, void **binder);

    // android::Composer::getBuiltInDisplay(android::Composer *this, int)
    extern int _ZN7android8Composer17getBuiltInDisplayEi(void *this, int32_t id);

    //  android::MemoryDealer::MemoryDealer(android::MemoryDealer *this, unsigned int, const char *, unsigned int)
    extern void* _ZN7android12MemoryDealerC2EjPKcj(void *this, uint32_t size, const char* name, uint32_t flags);


//code exports we provide

    // android::SurfaceControl::setLayer(unsigned int)
    int _ZN7android14SurfaceControl8setLayerEi(void *surfaceControl, int32_t layer);

    // android::SurfaceControl::setPosition(int x, int y)
    int _ZN7android14SurfaceControl11setPositionEii(void *surfaceControl, int32_t x, int32_t y);

    // android::SurfaceComposerClient::getDisplayInfo(int, android::DisplayInfo *)
    int _ZN7android21SurfaceComposerClient14getDisplayInfoEiPNS_11DisplayInfoE(int32_t a1, int32_t a2, int32_t a3);

    // android::SurfaceComposerClient::createSurface(int32_t display, uint32_t w, uint32_t h, PixelFormat format, uint32_t flags)
    void* _ZN7android21SurfaceComposerClient13createSurfaceEijjij(
        void *surfaceComposerClient,
        int32_t display,
        uint32_t w,
        uint32_t h,
        int32_t format,
        uint32_t flags);

    // android::Composer::setOrientation(int)
    int _ZN7android8Composer14setOrientationEi(void *this, int orientation);

    // android::SurfaceComposerClient::setOrientation(int32_t dpy, int orientation, uint32_t flags);
    int _ZN7android21SurfaceComposerClient14setOrientationEiij(void *this, int32_t dpy, int32_t orientation, uint32_t flags);

    // android::MemoryDealer::MemoryDealer(android::MemoryDealer *this, unsigned int, const char *)
    void* _ZN7android12MemoryDealerC1EjPKc(void *this, uint32_t size, const char* name);


//library on-load and on-unload handlers (to help us set things up and tear them down)
    void libEvtLoading(void) __attribute__((constructor));
    void libEvtUnloading(void) __attribute__((destructor));

/*
 * FUNCTION:    android::SurfaceComposerClient::getDisplayInfo(int, android::DisplayInfo *)
 *
 */
int _ZN7android21SurfaceComposerClient14getDisplayInfoEiPNS_11DisplayInfoE(int32_t a1, int32_t a2, int32_t a3) {

    ALOGI("_ZN7android21SurfaceComposerClient14getDisplayInfoEiPNS_11DisplayInfoE");
    int res;
    void *instance = &a2;
    _ZN7android21SurfaceComposerClient17getBuiltInDisplayEi(&instance, a1);
     res = _ZN7android21SurfaceComposerClient14getDisplayInfoERKNS_2spINS_7IBinderEEEPNS_11DisplayInfoE(
        &instance,
        instance);
    _ZN7android2spINS_7IBinderEED2Ev(&instance);
    return res;
}

/*
 * FUNCTION:    android::SurfaceControl::setLayer(unsigned int)
 * USE:         libnvwinsys.so
 */
int _ZN7android14SurfaceControl8setLayerEi(void *surfaceControl, int32_t layer) {
    ALOGI("_ZN7android14SurfaceControl8setLayerEi");
    return _ZN7android14SurfaceControl8setLayerEj(surfaceControl, (uint32_t)layer);
}

/*
 * FUNCTION:    android::SurfaceControl::setPosition(int x, int y)
 * USE:         libnvwinsys.so
 */
int _ZN7android14SurfaceControl11setPositionEii(void *surfaceControl, int32_t x, int32_t y) {
    ALOGI("_ZN7android14SurfaceControl11setPositionEii");
    return _ZN7android14SurfaceControl11setPositionEff(surfaceControl, (float)x, (float)y);
}

/*
 * FUNCTION:     android::SurfaceComposerClient::createSurface(android::String8 const&,uint,uint,int,uint)
 * USE:
 */
void* _ZN7android21SurfaceComposerClient13createSurfaceEijjij(
        void *surfaceComposerClient,
        int32_t display,
        uint32_t w,
        uint32_t h,
        int32_t format,
        uint32_t flags)
{
    // String8 name;
    // const size_t SIZE = 128;
    // char buffer[SIZE];
    // snprintf(buffer, SIZE, "<pid_%d>", getpid());
    // name.append(buffer);

    ALOGI("_ZN7android21SurfaceComposerClient13createSurfaceEijjij");

    (void) display;

    void *name;
    _ZN7android7String8C1Ev(&name);
    const size_t SIZE = 128;
    char buffer[SIZE];
    snprintf(buffer, SIZE, "<pid_%d>", getpid());
    _ZN7android7String86appendEPKc(&name, (const char*)&buffer);
    _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8Ejjij(
        surfaceComposerClient,
        &name,
        w, h,
        format,
        flags);
    _ZN7android7String8D1Ev(&name);

    return surfaceComposerClient;
}

/*
 * FUNCTION:  android::Composer::setOrientation(int)
 * USE:  libnvwinsys.so
 */
int _ZN7android8Composer14setOrientationEi(void *this, int orientation)
{
    void* composerService;
    void* token;
    void* dispState;

    // sp<ISurfaceComposer> sm(ComposerService::getComposerService());
    composerService = _ZN7android15ComposerService18getComposerServiceEv(this);

    // sp<IBinder> token(sm->getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain))
    typedef void* getBuiltInDisplay(void*);
    getBuiltInDisplay* f = (void*)(this + 36);
    f(token);

    // DisplayState& s(getDisplayStateLocked(token));
    dispState = _ZN7android8Composer21getDisplayStateLockedERKNS_2spINS_7IBinderEEE(this, &token);

    // s.orientation = orientation;
    *((intptr_t*)(dispState + 16)) = orientation;

    // mForceSynchronous = true; 
    *((int8_t*)(this + 44)) = 1;

    _ZN7android2spINS_7IBinderEED2Ev(&token);
    _ZN7android2spINS_7IBinderEED2Ev(&composerService);

    return 0;
}

/*
 * FUNCTION:  android::SurfaceComposerClient::setOrientation(int32_t dpy, int32_t orientation, uint32_t flags);
 * USE:  libnvwinsys.so
 */
int _ZN7android21SurfaceComposerClient14setOrientationEiij(void *this, int32_t dpy, int32_t orientation, uint32_t flags)
{
    (void) dpy;
    (void) flags;
    void* instance;
     instance = _ZN7android9SingletonINS_8ComposerEE11getInstanceEv(this);
    return _ZN7android8Composer14setOrientationEi(&instance, orientation);
}

/*
 * FUNCTION: android::MemoryDealer::MemoryDealer(android::MemoryDealer *this, unsigned int, const char *)
 * USE: libnvomxadaptor.so
 */
void* _ZN7android12MemoryDealerC1EjPKc(void *this, uint32_t size, const char* name)
{
    void* instance = this;
    _ZN7android12MemoryDealerC2EjPKcj(instance, size, name, 0);
    return instance;
}

/*
 * FUNCTION: libEvtLoading()
 * USE:      Handle library loading
 * NOTES:    This is a good time to log the fact that we were loaded and plan to
 *           do our thing.
 */
void libEvtLoading(void)
{
    ALOGI("NVidia interposition library loaded.");
}

/*
 * FUNCTION: libEvtUnloading()
 * USE:      Handle library unloading
 * NOTES:    This is a good time to free whatever is unfreed and say goodbye
 */
void libEvtUnloading(void)
{
    ALOGI("NVidia interposition library unloaded.");
}
