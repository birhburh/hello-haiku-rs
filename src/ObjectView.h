/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef OBJECT_VIEW_H
#define OBJECT_VIEW_H

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glu.h>
#include <GLView.h>
#include <GL/glext.h>

#define kMsgFPS			'fps '
#define kMsgLimitFps	'lfps'
#define kMsgAddModel	'addm'
#define kMsgGouraud		'gour'
#define kMsgZBuffer		'zbuf'
#define kMsgCulling		'cull'
#define kMsgTextured	'txtr'
#define kMsgFog			'fog '
#define kMsgLighting	'lite'
#define kMsgLights		'lits'
#define kMsgFilled		'fill'
#define kMsgPerspective	'prsp'

enum lights {
	lightNone = 0,
	lightWhite,
	lightYellow,
	lightRed,
	lightBlue,
	lightGreen
};

#define HISTSIZE 10

struct TrackingInfo {
	float		lastX;
	float		lastY;
	float		lastDx;
	float		lastDy;
	bool		isTracking;
	uint32		buttons;
};

class ObjectView : public BGLView {
	public:
		bool			fLimitFps;
						ObjectView(BRect rect, const char* name,
							ulong resizingMode, ulong options);
						~ObjectView();

		virtual	void	MouseDown(BPoint point);
		virtual	void	MouseUp(BPoint point);
		virtual	void	MouseMoved(BPoint point, uint32 transit, const BMessage *msg);

		virtual	void	MessageReceived(BMessage* msg);
		virtual	void	AttachedToWindow();
		virtual	void	DetachedFromWindow();
		virtual	void	FrameResized(float width, float height);
		virtual	void	DrawFrame(bool noPause);
		virtual	void	Pulse();
				void	EnforceState();
		sem_id			drawEvent;
		sem_id			quittingSem;

	private:
		thread_id		fDrawThread;
		uint64			fLastFrame;
		int32			fHistEntries,fOldestEntry;
		bool			fFps, fLastGouraud, fGouraud;
		bool			fLastZbuf, fZbuf, fLastCulling, fCulling;
		bool			fLastLighting, fLighting, fLastFilled, fFilled;
		bool			fLastPersp, fPersp, fLastTextured, fTextured;
		bool			fLastFog, fFog, fForceRedraw;
		float			fLastYXRatio, fYxRatio, fFpsHistory[HISTSIZE];
		float			fObjectDistance, fLastObjectDistance;
		TrackingInfo	fTrackingInfo;
		unsigned int VAO;
		unsigned int VBO;
    		unsigned int vertexShader = 0;
    		unsigned int fragmentShader = 0;
    		unsigned int shaderProgram = 0;
};

#endif // OBJECT_VIEW_H
