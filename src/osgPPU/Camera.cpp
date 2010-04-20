/***************************************************************************
*   Copyright (c) 2010   Art Tevs                                         *
*                                                                         *
*   This library is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Lesser General Public License as        *
*   published by the Free Software Foundation; either version 3 of        *
*   the License, or (at your option) any later version.                   *
*                                                                         *
*   This library is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU Lesse General Public License for more details.                    *
*                                                                         *
*   The full license is in LICENSE file included with this distribution.  *
***************************************************************************/

#include <osgPPU/Camera.h>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/TextureRectangle>
#include <osgViewer/Renderer>

namespace osgPPU
{
	//------------------------------------------------------------------------------
	void Camera::resizeViewport(int x, int y, int width, int height, osg::Camera* camera)
	{
		// reset viewport
		osg::Viewport* vp = new osg::Viewport(x,y,width,height);
		camera->setViewport(vp);

		// reset renderer for proper update of the FBO on the next apply
		osgViewer::Renderer* renderer = (osgViewer::Renderer*)camera->getRenderer();
		renderer->getSceneView(0)->getRenderStage()->setCameraRequiresSetUp(true);
		renderer->getSceneView(0)->getRenderStage()->setFrameBufferObject(NULL);

		// resize texture attachments of the camera
		for(osg::Camera::BufferAttachmentMap::iterator it = camera->getBufferAttachmentMap().begin(); it != camera->getBufferAttachmentMap().end(); it++)
		{
			osg::Texture* texture = it->second._texture.get();

			if (texture == NULL) continue;

			// if texture type is a 2d texture
			if (dynamic_cast<osg::Texture2D*>(texture) != NULL)
			{
				// change size
				osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>(texture);
				tex->setTextureSize(int(vp->width()), int(vp->height()) );
				tex->dirtyTextureObject();
			}
			// if texture type is rectangle
			else if (dynamic_cast<osg::TextureRectangle*>(texture) != NULL)
			{
				// change size
				osg::TextureRectangle* tex = dynamic_cast<osg::TextureRectangle*>(texture);
				tex->setTextureSize(int(vp->width()), int(vp->height()) );
				tex->dirtyTextureObject();
			}
			// if texture type is a cubemap texture
			else if (dynamic_cast<osg::TextureCubeMap*>(texture) != NULL)
			{
				// change size
				osg::TextureCubeMap* tex = dynamic_cast<osg::TextureCubeMap*>(texture);
				tex->setTextureSize(int(vp->width()), int(vp->height()) );
				tex->dirtyTextureObject();
			}
			// if texture type is a 3d texture
			else if (dynamic_cast<osg::Texture3D*>(texture) != NULL)
			{
				// change size
				osg::Texture3D* tex = dynamic_cast<osg::Texture3D*>(texture);
				tex->setTextureSize(int(vp->width()), int(vp->height()), tex->getTextureDepth() );
				tex->dirtyTextureObject();
			}
			// if texture type is a 2d array
			else if (dynamic_cast<osg::Texture2DArray*>(texture) != NULL)
			{
				// change size
				osg::Texture2DArray* tex = dynamic_cast<osg::Texture2DArray*>(texture);
				tex->setTextureSize(int(vp->width()), int(vp->height()), tex->getTextureDepth() );
				tex->dirtyTextureObject();
			}
			// unknown textue
			else
			{
				osg::notify(osg::WARN) << "osgPPU::Camera::resizeViewport() - non-supported texture type specified!" << std::endl;
			}
		}
	}

}
