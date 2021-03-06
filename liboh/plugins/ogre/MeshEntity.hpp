/*  Sirikata Graphical Object Host
 *  MeshEntity.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#ifndef SIRIKATA_GRAPHICS_MESHOBJECT_HPP__
#define SIRIKATA_GRAPHICS_MESHOBJECT_HPP__

#include <oh/Platform.hpp>

#include "options/Options.hpp"
#include "OgreSystem.hpp"
#include "OgrePlugin.hpp"

#include <oh/ProxyMeshObject.hpp>
#include <oh/MeshListener.hpp>
#include "Entity.hpp"
#include <OgreEntity.h>
#include "resourceManager/GraphicsResourceEntity.hpp"

namespace Sirikata {
namespace Graphics {

class WebView;

class MeshEntity
    :   public Entity,
        public MeshListener
{
public:
    typedef std::tr1::shared_ptr<Meru::GraphicsResourceEntity> SharedResourcePtr;
    typedef std::map<int, std::pair<String, Ogre::MaterialPtr> > ReplacedMaterialMap;
    typedef std::map<String, String > TextureBindingsMap;
private:
    SharedResourcePtr mResource;
    BoundingInfo mBoundingInfo;

    ReplacedMaterialMap mReplacedMaterials;
    TextureBindingsMap mTextureBindings;

    Ogre::Entity *getOgreEntity() const {
        return static_cast<Ogre::Entity*const>(mOgreObject);
    }

    void fixTextures();

public:
    ProxyMeshObject &getProxy() const {
        return *std::tr1::static_pointer_cast<ProxyMeshObject>(mProxy);
    }
    MeshEntity(OgreSystem *scene,
               const std::tr1::shared_ptr<ProxyMeshObject> &pmo,
               const std::string&ogre_id=std::string());

    virtual ~MeshEntity();

//    Vector3f getScale() const {
//        return fromOgre(mSceneNode->getScale());
//    }
    void bindTexture(const std::string &textureName, const SpaceObjectReference &objId);
    void unbindTexture(const std::string &textureName);

    WebView *getWebView(int whichSubEnt);

    static std::string ogreMeshName(const SpaceObjectReference&ref);
    virtual std::string ogreMovableName()const;

    /** Load the mesh and use it for this entity
     *  \param meshname the name (ID) of the mesh to use for this entity
     */
    void loadMesh(const String& meshname);

    void unloadMesh();

    virtual void setSelected(bool selected);

    ///Returns the scaled bounding info
    const BoundingInfo& getBoundingInfo()const{
        return mBoundingInfo;
    }
    const SharedResourcePtr &getResource() const {
        return mResource;
    }
    void setResource(const SharedResourcePtr &resourcePtr) {
        mResource = resourcePtr;
    }

/*
    virtual bool loadMesh(const String&name){
        return false;
    }
  */

    // interface from MeshListener
    public:
        virtual void onSetMesh ( URI const& meshFile );
        virtual void onSetScale ( Vector3f const& scale );
        virtual void onSetPhysical ( PhysicalParameters const& pp );
    
    protected:    
    
};

}
}

#endif
