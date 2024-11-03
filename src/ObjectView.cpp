#include <iostream>

#include "ObjectView.h"

#include <Application.h>
#include <Catalog.h>
#include <Cursor.h>
#include <InterfaceKit.h>
#include <FindDirectory.h>

#include "FPS.h"

float displayScale = 1.0;
float depthOfView = 30.0;
float zRatio = 10.0;

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
	printf("[OpenGL Renderer]          %s\n", glGetString(GL_RENDERER));
	printf("[OpenGL Vendor]            %s\n", glGetString(GL_VENDOR));
	printf("[OpenGL Version]           %s\n", glGetString(GL_VERSION));
	GLint profile;	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
	printf("[OpenGL Profile]           %s\n", profile ? "Core" : "Compatibility");
	printf("[OpenGL Shading Language]  %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
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

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor; \n"
"void main() \n"
"{\n"
"    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); \n"
"}\n\0";


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

	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);

	glViewport(0, 0, (GLint)bounds.IntegerWidth() + 1,
				(GLint)bounds.IntegerHeight() + 1);
	this->shaderProgram = glCreateProgram();
        this->vertexShader = glCreateShader(GL_VERTEX_SHADER);;
	glShaderSource(this->vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(this->vertexShader);
	int success;
    	char infoLog[512];
    	glGetShaderiv(this->vertexShader, GL_COMPILE_STATUS, &success);

    	if (!success)
    	{
    	    glGetShaderInfoLog(this->vertexShader, 512, NULL, infoLog);
    	    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    	}
	GLint vertSrcLen;

        this->fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);;
	glShaderSource(this->fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(this->fragmentShader);
	glGetShaderiv(this->fragmentShader, GL_COMPILE_STATUS, &success);

    	if (!success)
    	{
    	    glGetShaderInfoLog(this->fragmentShader, 512, NULL, infoLog);
    	    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    	}

        glAttachShader(this->shaderProgram, this->vertexShader);
        glAttachShader(this->shaderProgram, this->fragmentShader);
        glLinkProgram(this->shaderProgram);
	glGetProgramiv(this->shaderProgram, GL_LINK_STATUS, &success);
    	if (!success) {
    	    glGetProgramInfoLog(this->shaderProgram, 512, NULL, infoLog);
    	    std::cout << "ERROR::SHADER::Program::COMPILATION_FAILED\n" << infoLog << std::endl;
    	}

	// set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,  // left
             0.5f, -0.5f, 0.0f,  // right
             0.0f,  0.5f, 0.0f   // top
        };

        // ..:: Initialization code (done once (unless your object frequently changes)) :: ..
        // 1. bind Vertex Array Object
        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &this->VBO);

        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(this->VAO);

        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);

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

	UnlockGL();

	fForceRedraw = true;
	setEvent(drawEvent);
}

void
ObjectView::DrawFrame(bool noPause)
{
	LockGL();
	glClear(GL_COLOR_BUFFER_BIT | (fZbuf ? GL_DEPTH_BUFFER_BIT : 0));
	glClearColor(1.0, 0.0, 0.0, 1.0);
	
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
		
		glClearColor(0.0, 1.0, 0.0, 1.0);
		glUseProgram(this->shaderProgram);
    		glBindVertexArray(this->VAO);
    		glDrawArrays(GL_TRIANGLES, 0, 3);

		if (fHistEntries > 5) {
			fps = 0;
			for (int i = 0; i < fHistEntries; i++)
				fps += fFpsHistory[(fOldestEntry + i) % HISTSIZE];

			fps /= fHistEntries;

			if (true) {
				glUseProgram(0);
				glColor4f(1, 1, 1, 1);
				glPushMatrix();
    				glMatrixMode(GL_MODELVIEW);
    				glLoadIdentity();
    				glTranslatef(-0.9f, -0.9f, 0.0f);
    				glScalef(0.1f, 0.1f, 0.0f);
				FPS::drawCounter(fps);
				glPopMatrix();
			}
		}
	} else {
		fHistEntries = 0;
		fOldestEntry = 0;
	}
	SwapBuffers();
	UnlockGL();
}
