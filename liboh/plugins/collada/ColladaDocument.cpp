/*  Sirikata liboh -- COLLADA Models Document
 *  ColladaDocument.cpp
 *
 *  Copyright (c) 2009, Mark C. Barnes
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

#include "ColladaDocument.hpp"

#include <cassert>
#include <iostream>

namespace Sirikata { namespace Models {

ColladaDocument::ColladaDocument ( Transfer::URI const& uri )
    :   mURI ( uri ),
        mAsset()
{
    assert((std::cout << "MCB: ColladaDocument::ColladaDocument() entered" << std::endl,true));
    
}


ColladaDocument::~ColladaDocument ()
{
    assert((std::cout << "MCB: ColladaDocument::~ColladaDocument() entered" << std::endl,true));
    
}
    
/////////////////////////////////////////////////////////////////////

bool ColladaDocument::import ( ColladaDocumentImporter& importer, COLLADAFW::FileInfo const& asset )
{
    assert((std::cout << "MCB: ColladaDocument::import(COLLADAFW::FileInfo) entered" << std::endl,true));

    bool ok = mAsset.import ( importer, asset );

    return ok;
}

/////////////////////////////////////////////////////////////////////

bool ColladaDocument::import ( ColladaDocumentImporter& importer, COLLADAFW::Geometry const& geometry )
{
    assert((std::cout << "MCB: ColladaDocument::import(COLLADAFW::Geometry) entered" << std::endl,true));

    // MCB: I don't like this approach. It's too tightly coupled to MeshObject.
    // MCB: How about a set of ColladaGeometry objects here instead?
//    bool ok = mMeshObject.import ( importer, geometry );
    bool ok = true;

    return ok;
}

/////////////////////////////////////////////////////////////////////
    
/////////////////////////////////////////////////////////////////////
    

} // namespace Models
} // namespace Sirikata
