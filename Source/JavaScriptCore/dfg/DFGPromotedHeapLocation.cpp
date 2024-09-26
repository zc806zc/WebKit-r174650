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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DFGPromotedHeapLocation.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "JSCInlines.h"

namespace JSC { namespace DFG {

void PromotedLocationDescriptor::dump(PrintStream& out) const
{
    out.print(m_kind, "(", m_info, ")");
}

Node* PromotedHeapLocation::createHint(Graph& graph, NodeOrigin origin, Node* value)
{
    switch (kind()) {
    case StructurePLoc:
        return graph.addNode(
            SpecNone, PutStructureHint, origin,
            Edge(base(), KnownCellUse), Edge(value, KnownCellUse));
        
    case NamedPropertyPLoc:
        return graph.addNode(
            SpecNone, PutByOffsetHint, origin,
            OpInfo(info()), Edge(base(), KnownCellUse), Edge(value, UntypedUse));
        
    case InvalidPromotedLocationKind:
        return nullptr;
    }
    
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

void PromotedHeapLocation::dump(PrintStream& out) const
{
    out.print(kind(), "(", m_base, ", ", info(), ")");
}

} } // namespace JSC::DFG

namespace WTF {

using namespace JSC::DFG;

void printInternal(PrintStream& out, PromotedLocationKind kind)
{
    switch (kind) {
    case InvalidPromotedLocationKind:
        out.print("InvalidPromotedLocationKind");
        return;
        
    case StructurePLoc:
        out.print("StructurePLoc");
        return;
        
    case NamedPropertyPLoc:
        out.print("NamedPropertyPLoc");
        return;
    }
    
    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace WTF;

#endif // ENABLE(DFG_JIT)

