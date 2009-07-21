/*  Sirikata liboh -- Ogre Graphics Plugin
 *  InputEventDescriptor.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "InputEventDescriptor.hpp"

namespace Sirikata {
namespace Input {

EventDescriptor EventDescriptor::Key(KeyEventButton button, KeyEventType type, KeyEventModifier mod) {
    EventDescriptor result;
    result.mTag = KeyEventTag;
    result.mDescriptor.key.button = button;
    result.mDescriptor.key.type = type;
    result.mDescriptor.key.mod = mod;
    return result;
}

EventDescriptor EventDescriptor::MouseClick(MouseClickEventButton button) {
    EventDescriptor result;
    result.mTag = MouseClickEventTag;
    result.mDescriptor.mouseClick.button = button;
    return result;
}

EventDescriptor EventDescriptor::MouseDrag(MouseDragEventButton button) {
    EventDescriptor result;
    result.mTag = MouseDragEventTag;
    result.mDescriptor.mouseDrag.button = button;
    return result;
}

EventDescriptor EventDescriptor::Axis(AxisIndex index) {
    EventDescriptor result;
    result.mTag = AxisEventTag;
    result.mDescriptor.axis.index = index;
    return result;
}

bool EventDescriptor::isKey() const {
    return mTag == KeyEventTag;
}

EventDescriptor::KeyEventButton EventDescriptor::keyButton() const {
    assert(isKey());
    return mDescriptor.key.button;
}

EventDescriptor::KeyEventType EventDescriptor::keyEvents() const {
    assert(isKey());
    return mDescriptor.key.type;
}

EventDescriptor::KeyEventModifier EventDescriptor::keyModifiers() const {
    assert(isKey());
    return mDescriptor.key.mod;
}

bool EventDescriptor::isMouseClick() const {
    return mTag == MouseClickEventTag;
}

EventDescriptor::MouseClickEventButton EventDescriptor::mouseClickButton() const {
    assert(isMouseClick());
    return mDescriptor.mouseClick.button;
}

bool EventDescriptor::isMouseDrag() const {
    return mTag == MouseDragEventTag;
}

EventDescriptor::MouseDragEventButton EventDescriptor::mouseDragButton() const {
    assert(isMouseDrag());
    return mDescriptor.mouseDrag.button;
}

bool EventDescriptor::isAxis() const {
    return mTag == AxisEventTag;
}

EventDescriptor::AxisIndex EventDescriptor::axisIndex() const {
    assert(isAxis());
    return mDescriptor.axis.index;
}

bool EventDescriptor::operator<(const EventDescriptor& rhs) const {
    // Evaluate easy comparisons first
    if (mTag < rhs.mTag) return true;
    if (rhs.mTag < mTag) return false;

    // Otherwise, types are the same so we need to do detailed checks
    if (mTag == KeyEventTag)
        return
            mDescriptor.key.button < rhs.mDescriptor.key.button ||
            (mDescriptor.key.button == rhs.mDescriptor.key.button &&
                (mDescriptor.key.type < rhs.mDescriptor.key.type ||
                (mDescriptor.key.type == rhs.mDescriptor.key.type &&
                    mDescriptor.key.mod < rhs.mDescriptor.key.mod
                )
                )
            );

    if (mTag == MouseClickEventTag)
        return mDescriptor.mouseClick.button < rhs.mDescriptor.mouseClick.button;

    if (mTag == MouseDragEventTag)
        return mDescriptor.mouseDrag.button < rhs.mDescriptor.mouseDrag.button;

    if (mTag == AxisEventTag)
        return mDescriptor.axis.index < rhs.mDescriptor.axis.index;

    assert(false); // we should have checked all types of tags by now
}

} // namespace Input
} // namespace Sirikata