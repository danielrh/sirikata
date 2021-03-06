/*  Sirikata Graphical Object Host
 *  MonoPlugin.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include <oh/Platform.hpp>
#include "MonoDefs.hpp"
#include "MonoDomain.hpp"
#include "MonoSystem.hpp"
///these includes are for fiddling around
#include "MonoAssembly.hpp"
#include "MonoClass.hpp"
#include "MonoObject.hpp"
#include "MonoVWObjectScriptManager.hpp"
#include "MonoArray.hpp"
#include "oh/ObjectScriptManagerFactory.hpp"
#include "MonoException.hpp"
#include "MonoHostedObjectExports.hpp"
#include "MonoContext.hpp"
static int core_plugin_refcount = 0;
Mono::MonoSystem * mono_system;

bool loadDependencyAssembly(Mono::MonoSystem*mono_system, const Sirikata::String&assembly) {
    return   mono_system->loadAssembly(assembly,"../../../dependencies/IronPython")
        || mono_system->loadAssembly(assembly,"../../../dependencies/Cecil")
        ||mono_system->loadAssembly(assembly,"../../dependencies/IronPython")
        ||mono_system->loadAssembly(assembly,"../../dependencies/Cecil")
        ||mono_system->loadAssembly(assembly,".")
        ||mono_system->loadAssembly(assembly,"bin")
        ||mono_system->loadAssembly(assembly,"..")
        ||mono_system->loadAssembly(assembly,"lib")
        ||mono_system->loadAssembly(assembly,"../lib")
        ||mono_system->loadAssembly(assembly,"dependencies/IronPython")
        ||mono_system->loadAssembly(assembly,"dependencies/Cecil");
}
bool loadCustomAssembly(Mono::MonoSystem*mono_system, const Sirikata::String&assembly) {
    return   
            mono_system->loadAssembly(assembly,".")
            ||mono_system->loadAssembly(assembly,"bin")
            ||mono_system->loadAssembly(assembly,"lib")
            ||mono_system->loadAssembly(assembly,"../lib")
            ||mono_system->loadAssembly(assembly,"Release")
            ||mono_system->loadAssembly(assembly,"Debug")
            ||mono_system->loadAssembly(assembly,"..");//for XCode

}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    using std::tr1::placeholders::_1;
    if (core_plugin_refcount==0) {
        mono_system = new Mono::MonoSystem();
        ObjectScriptManagerFactory::getSingleton().registerConstructor("monoscript",
                                                                       std::tr1::bind(&Sirikata::MonoVWObjectScriptManager::createObjectScriptManager,mono_system,_1),
                                                                       true);
        MonoContext::getSingleton().initializeThread();
        Sirikata::InitHostedObjectExports();
        Mono::Domain d=mono_system->createDomain();
        bool retval=loadDependencyAssembly(mono_system,"Microsoft.Scripting");
        retval=loadDependencyAssembly(mono_system,"IronPython")&&retval;
        retval=loadDependencyAssembly(mono_system,"IronPython.Modules")&&retval;
 
        bool testretval=loadCustomAssembly(mono_system,"Sirikata.Runtime")&&
            loadCustomAssembly(mono_system,"Sirikata.Protocol");
        /*
        Mono::Assembly ass=d.getAssembly("Sirikata.Runtime");
        Mono::Class cls =ass.getClass("PythonObject");
        Mono::Object monoobj=cls.instance(d.Array(d.String(String()).type(),0));
        try {
            monoobj.send("processRPC",d.ByteArray("",0),d.String("rpctest"),d.ByteArray("",0));
            monoobj.send("tick",d.Time(Time::now()));
        }catch (Mono::Exception&e) {
            SILOG(mono,error,"Error processing rpc"<<e);
        }
        */
        printf ("Mono Initialized %d %d \n",(int) retval, (int) testretval);
        
    }
    core_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++core_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(core_plugin_refcount>0);
    return --core_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (core_plugin_refcount>0) {
        core_plugin_refcount--;
        assert(core_plugin_refcount==0);
        if (core_plugin_refcount==0) {
            ObjectScriptManagerFactory::getSingleton().unregisterConstructor("monoscript");
            delete mono_system;
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "mono";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}
