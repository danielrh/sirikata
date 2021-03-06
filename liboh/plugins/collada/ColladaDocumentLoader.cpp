/*  Sirikata liboh -- COLLADA Models Document Loader
 *  ColladaDocumentLoader.cpp
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

#include "ColladaDocumentLoader.hpp"

#include "ColladaErrorHandler.hpp"
#include "ColladaDocumentImporter.hpp"

//#include "ColladaMeshObject.hpp"

//#include <oh/ProxyMeshObject.hpp>
//#include <oh/SimulationFactory.hpp>
//#include <options/Options.hpp>
//#include <transfer/TransferManager.hpp>

// OpenCOLLADA headers
#include "COLLADAFWRoot.h"
#include "COLLADASaxFWLLoader.h"


#include <iostream>

namespace Sirikata { namespace Models {
    
ColladaDocumentLoader::ColladaDocumentLoader ( Transfer::URI const& uri )
    :   mErrorHandler ( new ColladaErrorHandler ),
        mSaxLoader ( new COLLADASaxFWL::Loader ( mErrorHandler ) ),
        mDocumentImporter ( new ColladaDocumentImporter ( uri ) ),
        mFramework ( new COLLADAFW::Root ( mSaxLoader, mDocumentImporter ) )
{
    assert((std::cout << "MCB: ColladaDocumentLoader::ColladaDocumentLoader() entered" << std::endl,true));
    
}
    
ColladaDocumentLoader::~ColladaDocumentLoader ()
{
    assert((std::cout << "MCB: ColladaDocumentLoader::~ColladaDocumentLoader() entered" << std::endl,true));
    
    delete mFramework;
    delete mDocumentImporter;
    delete mSaxLoader;
    delete mErrorHandler;
}
    
/////////////////////////////////////////////////////////////////////

bool ColladaDocumentLoader::load ( char const* buffer, size_t bufferLength )
{
    bool ok = mFramework->loadDocument ( getDocument()->getURI().toString(), buffer, bufferLength );

    return ok;
}


ColladaDocumentPtr ColladaDocumentLoader::getDocument () const
{
    return mDocumentImporter->getDocument ();
}

/////////////////////////////////////////////////////////////////////


} // namespace Models
} // namespace Sirikata
