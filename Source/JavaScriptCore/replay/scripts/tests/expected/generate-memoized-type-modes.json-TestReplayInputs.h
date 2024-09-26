/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// DO NOT EDIT THIS FILE. It is automatically generated from generate-memoized-type-modes.json
// by the script: JavaScriptCore/replay/scripts/CodeGeneratorReplayInputs.py

#ifndef generate_memoized_type_modes_json_TestReplayInputs_h
#define generate_memoized_type_modes_json_TestReplayInputs_h

#if ENABLE(WEB_REPLAY)
#include "InternalNamespaceHeaderIncludeDummy.h"
#include <platform/ExternalNamespaceHeaderIncludeDummy.h>



namespace Test {
class ScalarInput;
class MapInput;
} // namespace Test

namespace JSC {
template<> struct InputTraits<Test::ScalarInput> {
    static InputQueue queue() { return InputQueue::ScriptMemoizedData; }
    static const String& type();

    static void encode(JSC::EncodedValue&, const Test::ScalarInput&);
    static bool decode(JSC::EncodedValue&, std::unique_ptr<Test::ScalarInput>&);
};

template<> struct InputTraits<Test::MapInput> {
    static InputQueue queue() { return InputQueue::ScriptMemoizedData; }
    static const String& type();

    static void encode(JSC::EncodedValue&, const Test::MapInput&);
    static bool decode(JSC::EncodedValue&, std::unique_ptr<Test::MapInput>&);
};

} // namespace JSC

namespace Test {
class ScalarInput : public NondeterministicInput<ScalarInput> {
public:
    ScalarInput(ScalarType data);
    virtual ~ScalarInput();

    ScalarType data() const { return m_data; }
private:
    ScalarType m_data;
};

class MapInput : public NondeterministicInput<MapInput> {
public:
    MapInput(std::unique_ptr<MapType> data);
    virtual ~MapInput();

    const MapType& data() const { return *m_data; }
private:
    std::unique_ptr<MapType> m_data;
};
} // namespace Test

#define TEST_REPLAY_INPUT_NAMES_FOR_EACH(macro) \
    macro(ScalarInput) \
    macro(MapInput) \
    \
// end of TEST_REPLAY_INPUT_NAMES_FOR_EACH

#endif // ENABLE(WEB_REPLAY)

#endif // generate-memoized-type-modes.json-TestReplayInputs_h
