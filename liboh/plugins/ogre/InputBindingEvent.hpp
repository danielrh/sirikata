/*  Sirikata liboh -- Ogre Graphics Plugin
 *  InputBindingEvent.hpp
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

#ifndef _SIRIKATA_INPUT_BINDING_EVENT_HPP_
#define _SIRIKATA_INPUT_BINDING_EVENT_HPP_

#include <util/Platform.hpp>
#include "input/InputEvents.hpp"
#include "input/InputEventDescriptor.hpp"

namespace Sirikata {
namespace Graphics {

class InputBindingEvent {
public:
    static InputBindingEvent Key(Input::KeyButton button, Input::Modifier mod = Input::MOD_NONE);
    static InputBindingEvent MouseClick(Input::MouseButton button);
    static InputBindingEvent MouseDrag(Input::MouseButton button);
    static InputBindingEvent Axis(Input::AxisIndex axis);
    static InputBindingEvent Web(const String& wvname, const String& name, uint32 argcount = 0);

    InputBindingEvent();
    InputBindingEvent(const InputBindingEvent& rhs);
    ~InputBindingEvent();

    bool isKey() const;
    Input::KeyButton keyButton() const;
    Input::Modifier keyModifiers() const;

    bool isMouseClick() const;
    Input::MouseButton mouseClickButton() const;

    bool isMouseDrag() const;
    Input::MouseButton mouseDragButton() const;

    bool isAxis() const;
    Input::AxisIndex axisIndex() const;

    bool isWeb() const;
    const String& webViewName() const;
    const String& webName() const;
    uint32 webArgCount() const;

    InputBindingEvent& operator=(const InputBindingEvent& rhs);
private:
    Input::EventTypeTag mTag;

    union {
        struct {
            Input::KeyButton button;
            Input::Modifier mod;
        } key;
        struct {
            Input::MouseButton button;
        } mouseClick;
        struct {
            Input::MouseButton button;
        } mouseDrag;
        struct {
            Input::AxisIndex index;
        } axis;
        struct {
            String* wvname;
            String* name;
            uint32 argcount;
        } web;
    } mDescriptor;
};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_INPUT_BINDING_EVENT_HPP_
