/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner
 *
 */

/*
 * Original Be Sample source modified to use a quaternion for the object's orientation
 */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ObjectView.h"

#include <Application.h>
#include <Catalog.h>
#include <Cursor.h>
#include <InterfaceKit.h>
#include <FindDirectory.h>

#include "FPS.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ObjectView"

float displayScale = 1.0;
float depthOfView = 30.0;
float zRatio = 10.0;

const char *kNoResourceError = B_TRANSLATE("The Teapot 3D model was "
									"not found in application resources. "
									"Please repair the program installation.");

struct light {
	float *ambient;
	float *diffuse;
	float *specular;
};


long
signalEvent(sem_id event)
{
	int32 c;
	get_sem_count(event,&c);
	if (c < 0)
		release_sem_etc(event,-c,0);

	return 0;
}


long
setEvent(sem_id event)
{
	int32 c;
	get_sem_count(event,&c);
	if (c < 0)
	  release_sem_etc(event,-c,0);

	return 0;
}


long
waitEvent(sem_id event)
{
	acquire_sem(event);

	int32 c;
	get_sem_count(event,&c);
	if (c > 0)
		acquire_sem_etc(event,c,0,0);

	return 0;
}


static int32
simonThread(void* cookie)
{
	ObjectView* objectView = reinterpret_cast<ObjectView*>(cookie);
	BScreen screen(objectView->Window());

	int noPause = 0;
	while (acquire_sem_etc(objectView->quittingSem, 1, B_TIMEOUT, 0) == B_NO_ERROR) {
		if (true) {
			objectView->DrawFrame(noPause);
			release_sem(objectView->quittingSem);
			noPause = 1;
		} else {
			release_sem(objectView->quittingSem);
			noPause = 0;
			waitEvent(objectView->drawEvent);
		}
		if (objectView->fLimitFps)
			screen.WaitForRetrace();
	}
	return 0;
}


ObjectView::ObjectView(BRect rect, const char *name, ulong resizingMode,
	ulong options)
	: BGLView(rect, name, resizingMode, 0, options),
	fLimitFps(true),
	fHistEntries(0),
	fOldestEntry(0),
	fFps(true),
	fLastGouraud(true),
	fGouraud(true),
	fLastZbuf(true),
	fZbuf(true),
	fLastCulling(true),
	fCulling(true),
	fLastLighting(true),
	fLighting(true),
	fLastFilled(true),
	fFilled(true),
	fLastPersp(false),
	fPersp(false),
	fLastTextured(false),
	fTextured(false),
	fLastFog(false),
	fFog(false),
	fForceRedraw(false),
	fLastYXRatio(1),
	fYxRatio(1)
{
	fTrackingInfo.isTracking = false;
	fTrackingInfo.buttons = 0;
	fTrackingInfo.lastX = 0.0f;
	fTrackingInfo.lastY = 0.0f;
	fTrackingInfo.lastDx = 0.0f;
	fTrackingInfo.lastDy = 0.0f;

	fLastObjectDistance = fObjectDistance = depthOfView / 8;
	quittingSem = create_sem(1, "quitting sem");
	drawEvent = create_sem(0, "draw event");
}


ObjectView::~ObjectView()
{
	delete_sem(quittingSem);
	delete_sem(drawEvent);
}


void
ObjectView::AttachedToWindow()
{
	BRect bounds = Bounds();

	BGLView::AttachedToWindow();
	Window()->SetPulseRate(100000);

	LockGL();

	glEnable(GL_DITHER);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	glShadeModel(GL_SMOOTH);

	glFrontFace(GL_CW);
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);

	glMaterialf(GL_FRONT, GL_SHININESS, 0.6 * 128.0);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glColor3f(1.0, 1.0, 1.0);

	glViewport(0, 0, (GLint)bounds.IntegerWidth() + 1,
				(GLint)bounds.IntegerHeight() + 1);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float scale = displayScale;
	glOrtho(-scale, scale, -scale, scale, -scale * depthOfView,
			scale * depthOfView);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	UnlockGL();

	fDrawThread = spawn_thread(simonThread, "Simon", B_NORMAL_PRIORITY, this);
	resume_thread(fDrawThread);
	fForceRedraw = true;
	setEvent(drawEvent);
}


void
ObjectView::DetachedFromWindow()
{
	BGLView::DetachedFromWindow();

	status_t dummy;
	long locks = 0;

	while (Window()->IsLocked()) {
		locks++;
		Window()->Unlock();
	}

	acquire_sem(quittingSem);
	release_sem(drawEvent);
	wait_for_thread(fDrawThread, &dummy);
	release_sem(quittingSem);

	while (locks--)
		Window()->Lock();
}


void
ObjectView::Pulse()
{
	Window()->Lock();
	BRect parentBounds = Parent()->Bounds();
	BRect bounds = Bounds();
	parentBounds.OffsetTo(0, 0);
	bounds.OffsetTo(0, 0);
	if (bounds != parentBounds) {
		ResizeTo(parentBounds.right - parentBounds.left,
				 parentBounds.bottom - parentBounds.top);
	}
	Window()->Unlock();
}


void
ObjectView::MessageReceived(BMessage* msg)
{
	BMenuItem* item = NULL;
	bool toggleItem = false;

	switch (msg->what) {
		case kMsgFPS:
			fFps = (fFps) ? false : true;
			msg->FindPointer("source", reinterpret_cast<void**>(&item));
			item->SetMarked(fFps);
			fForceRedraw = true;
			setEvent(drawEvent);
			break;
		case kMsgAddModel: 
		{
			setEvent(drawEvent);
			break;
		}
		case kMsgLimitFps:
			fLimitFps = !fLimitFps;
			toggleItem = true;
			break;
	}

	if (toggleItem && msg->FindPointer("source", reinterpret_cast<void**>(&item)) == B_OK){
		item->SetMarked(!item->IsMarked());
		setEvent(drawEvent);
	}

	BGLView::MessageReceived(msg);
}


void
ObjectView::MouseDown(BPoint point)
{
	BMessage *msg = Window()->CurrentMessage();
}


void
ObjectView::MouseUp(BPoint point)
{
	if (fTrackingInfo.isTracking) {


		//stop tracking
		fTrackingInfo.isTracking = false;
		fTrackingInfo.buttons = 0;
		fTrackingInfo.lastX = 0.0f;
		fTrackingInfo.lastY = 0.0f;
		fTrackingInfo.lastDx = 0.0f;
		fTrackingInfo.lastDy = 0.0f;

		BCursor grabCursor(B_CURSOR_ID_GRAB);
		SetViewCursor(&grabCursor);
	}
}


void
ObjectView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	BCursor cursor(B_CURSOR_ID_SYSTEM_DEFAULT);
	SetViewCursor(&cursor);
}


void
ObjectView::FrameResized(float width, float height)
{
	BGLView::FrameResized(width, height);

	LockGL();

	width = Bounds().Width();
	height = Bounds().Height();
	fYxRatio = height / width;
	glViewport(0, 0, (GLint)width + 1, (GLint)height + 1);

	// To prevent weird buffer contents
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float scale = displayScale;

	if (fPersp) {
		gluPerspective(60, 1.0 / fYxRatio, 0.15, 120);
	} else {
		if (fYxRatio < 1) {
			glOrtho(-scale / fYxRatio, scale / fYxRatio, -scale, scale, -1.0,
				depthOfView * 4);
		} else {
			glOrtho(-scale, scale, -scale * fYxRatio, scale * fYxRatio, -1.0,
				depthOfView * 4);
		}
	}

	fLastYXRatio = fYxRatio;

	glMatrixMode(GL_MODELVIEW);

	UnlockGL();

	fForceRedraw = true;
	setEvent(drawEvent);
}


bool
ObjectView::RepositionView()
{
	if (!(fPersp != fLastPersp) &&
		!(fLastObjectDistance != fObjectDistance) &&
		!(fLastYXRatio != fYxRatio)) {
		return false;
	}

	LockGL();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float scale = displayScale;

	if (fPersp) {
		gluPerspective(60, 1.0 / fYxRatio, 0.15, 120);
	} else {
		if (fYxRatio < 1) {
			glOrtho(-scale / fYxRatio, scale / fYxRatio, -scale, scale, -1.0,
				depthOfView * 4);
		} else {
			glOrtho(-scale, scale, -scale * fYxRatio, scale * fYxRatio, -1.0,
				depthOfView * 4);
		}
	}

	glMatrixMode(GL_MODELVIEW);

	UnlockGL();

	fLastObjectDistance = fObjectDistance;
	fLastPersp = fPersp;
	fLastYXRatio = fYxRatio;
	return true;
}


void
ObjectView::EnforceState()
{
	glShadeModel(GL_FLAT);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

void
ObjectView::DrawFrame(bool noPause)
{
	LockGL();
	glClear(GL_COLOR_BUFFER_BIT | (fZbuf ? GL_DEPTH_BUFFER_BIT : 0));
	
	EnforceState();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	if (noPause) {
		uint64 now = system_time();
		float fps = 1.0 / ((now - fLastFrame) / 1000000.0);
		fLastFrame = now;
		int entry;
		if (fHistEntries < HISTSIZE) {
			entry = (fOldestEntry + fHistEntries) % HISTSIZE;
			fHistEntries++;
		} else {
			entry = fOldestEntry;
			fOldestEntry = (fOldestEntry + 1) % HISTSIZE;
		}

		fFpsHistory[entry] = fps;

		if (fHistEntries > 5) {
			fps = 0;
			for (int i = 0; i < fHistEntries; i++)
				fps += fFpsHistory[(fOldestEntry + i) % HISTSIZE];

			fps /= fHistEntries;

			if (fFps) {
				glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT);
				glPushMatrix();
				glLoadIdentity();
				glTranslatef(-0.9, -0.9, 0);
				glScalef(0.10, 0.10, 0.10);
				glDisable(GL_LIGHTING);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
				glColor3f(1.0, 1.0, 0);
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glMatrixMode(GL_MODELVIEW);

				FPS::drawCounter(fps);

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
				glPopAttrib();
			}
		}
	} else {
		fHistEntries = 0;
		fOldestEntry = 0;
	}
	SwapBuffers();
	UnlockGL();
}
