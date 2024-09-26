/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "TestController.h"

#import "CrashReporterInfo.h"
#import "PlatformWebView.h"
#import "PoseAsClass.h"
#import "TestInvocation.h"
#import "WebKitTestRunnerPasteboard.h"
#import <WebKit/WKStringCF.h>
#import <mach-o/dyld.h> 

@interface NSSound (Details)
+ (void)_setAlertType:(NSUInteger)alertType;
@end

namespace WTR {

void TestController::notifyDone()
{
}

void TestController::platformInitialize()
{
    poseAsClass("WebKitTestRunnerPasteboard", "NSPasteboard");
    poseAsClass("WebKitTestRunnerEvent", "NSEvent");

    [NSSound _setAlertType:0];
}

void TestController::platformDestroy()
{
    [WebKitTestRunnerPasteboard releaseLocalPasteboards];
}

void TestController::initializeInjectedBundlePath()
{
    NSString *nsBundlePath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"WebKitTestRunnerInjectedBundle.bundle"];
    m_injectedBundlePath.adopt(WKStringCreateWithCFString((CFStringRef)nsBundlePath));
}

void TestController::initializeTestPluginDirectory()
{
    m_testPluginDirectory.adopt(WKStringCreateWithCFString((CFStringRef)[[NSBundle mainBundle] bundlePath]));
}

void TestController::platformWillRunTest(const TestInvocation& testInvocation)
{
    setCrashReportApplicationSpecificInformationToURL(testInvocation.url());
}

static bool shouldUseThreadedScrolling(const char* pathOrURL)
{
    return strstr(pathOrURL, "tiled-drawing/");
}

void TestController::platformConfigureViewForTest(const TestInvocation& test)
{
    auto viewOptions = adoptWK(WKMutableDictionaryCreate());
    auto useThreadedScrollingKey = adoptWK(WKStringCreateWithUTF8CString("ThreadedScrolling"));
    auto useThreadedScrollingValue = adoptWK(WKBooleanCreate(shouldUseThreadedScrolling(test.pathOrURL())));
    WKDictionarySetItem(viewOptions.get(), useThreadedScrollingKey.get(), useThreadedScrollingValue.get());

    auto useRemoteLayerTreeKey = adoptWK(WKStringCreateWithUTF8CString("RemoteLayerTree"));
    auto useRemoteLayerTreeValue = adoptWK(WKBooleanCreate(shouldUseRemoteLayerTree()));
    WKDictionarySetItem(viewOptions.get(), useRemoteLayerTreeKey.get(), useRemoteLayerTreeValue.get());

    ensureViewSupportsOptions(viewOptions.get());
}

void TestController::platformRunUntil(bool& done, double timeout)
{
    NSDate *endDate = (timeout > 0) ? [NSDate dateWithTimeIntervalSinceNow:timeout] : [NSDate distantFuture];

    while (!done && [endDate compare:[NSDate date]] == NSOrderedDescending)
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:endDate];
}

void TestController::platformInitializeContext()
{
}

void TestController::setHidden(bool hidden)
{
    NSWindow *window = [mainWebView()->platformView() window];
    if (!window)
        return;

    if (hidden)
        [window orderOut:nil];
    else
        [window makeKeyAndOrderFront:nil];
}

void TestController::runModal(PlatformWebView* view)
{
    NSWindow *window = [view->platformView() window];
    if (!window)
        return;
    [NSApp runModalForWindow:window];
}

const char* TestController::platformLibraryPathForTesting()
{
    return [[@"~/Library/Application Support/DumpRenderTree" stringByExpandingTildeInPath] UTF8String];
}

} // namespace WTR
