/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "Test.h"
#include <WebKit/WKPagePrivate.h>

// This test loads file-with-video.html. It first checks to make sure WKPageIsPlayingAudio() returns
// false for the page. Then it calls a JavaScript method to play the video, waits for
// WKPageUIClient::isPlayingAudioDidChange() to get called, and checks that WKPageIsPlayingAudio() now
// returns true for the page.

namespace TestWebKitAPI {

static bool didFinishLoad;
static bool isPlayingAudioChanged;

static void nullJavaScriptCallback(WKSerializedScriptValueRef, WKErrorRef error, void*)
{
}

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef, WKTypeRef, const void*)
{
    didFinishLoad = true;
}

static void isPlayingAudioDidChangeCallback(WKPageRef page, const void*)
{
    isPlayingAudioChanged = true;
}

static void setUpClients(WKPageRef page)
{
    WKPageLoaderClientV0 loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));

    loaderClient.base.version = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;

    WKPageSetPageLoaderClient(page, &loaderClient.base);

    WKPageUIClientV4 uiClient;
    memset(&uiClient, 0, sizeof(uiClient));

    uiClient.base.version = 4;
    uiClient.isPlayingAudioDidChange = isPlayingAudioDidChangeCallback;

    WKPageSetPageUIClient(page, &uiClient.base);
}

TEST(WebKit2, WKPageIsPlayingAudio)
{
    WKRetainPtr<WKContextRef> context = adoptWK(WKContextCreate());

    PlatformWebView webView(context.get());
    setUpClients(webView.page());

    WKRetainPtr<WKURLRef> url = adoptWK(Util::createURLForResource("file-with-video", "html"));
    WKPageLoadURL(webView.page(), url.get());

    Util::run(&didFinishLoad);

    EXPECT_FALSE(WKPageIsPlayingAudio(webView.page()));
    WKPageRunJavaScriptInMainFrame(webView.page(), Util::toWK("playVideo()").get(), 0, nullJavaScriptCallback);

    Util::run(&isPlayingAudioChanged);
    EXPECT_TRUE(WKPageIsPlayingAudio(webView.page()));
}

} // namespace TestWebKitAPI
