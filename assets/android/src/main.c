#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, \
	"native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, \
	"native-activity", __VA_ARGS__))

struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

struct engine {
	struct android_app* app;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

static int engine_init_display(struct engine* engine) {
	const EGLint attribs[] = {
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs = 0;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, NULL, NULL);

	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	if (numConfigs == 0 || config == NULL) {
		LOGW("Unable to initialize EGLConfig");
		return -1;
	}

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;

	return 0;
}

static void engine_draw_frame(struct engine* engine) {
	if (engine->display == NULL) return;

	// Just fill the screen with a color.
	glClearColor(
		((float)engine->state.x) / engine->width,
		engine->state.angle * 0.1f,
		((float)engine->state.y)/engine->height,
		1);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(engine->display, engine->surface);
}

static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->animating = 1;
		engine->state.x = AMotionEvent_getX(event, 0);
		engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
	}
	return 0;
}

static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = app->userData;
	switch (cmd) {
		case APP_CMD_SAVE_STATE:
			// The system has asked us to save our current state.  Do so.
			engine->app->savedState = malloc(sizeof(struct saved_state));
			*((struct saved_state*)engine->app->savedState) = engine->state;
			engine->app->savedStateSize = sizeof(struct saved_state);
			break;
		case APP_CMD_INIT_WINDOW:
			// The window is being shown, get it ready.
			if (engine->app->window != NULL) {
				engine_init_display(engine);
				engine_draw_frame(engine);
			}
			break;
		case APP_CMD_TERM_WINDOW:
			// The window is being hidden or closed, clean it up.
			engine_term_display(engine);
			break;
		case APP_CMD_GAINED_FOCUS:
			break;
		case APP_CMD_LOST_FOCUS:
			// Also stop animating.
			engine->animating = 0;
			engine_draw_frame(engine);
			break;
		default:
			break;
	}
}

void android_main(struct android_app* state) {
	struct engine engine = { 0 };

	// Make sure glue isn't stripped.
	app_dummy();

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

	if (state->savedState != NULL) {
		// We are starting with a previous saved state; restore from it.
		engine.state = *(struct saved_state*)state->savedState;
	}

	while (true) {
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then
		// continue to draw the next frame of animation.
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL,
				&events, (void**)&source)) >= 0) {

			// Process this event.
			if (source != NULL) {
				source->process(state, source);
			}

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// Done with events; draw next animation frame.
			engine.state.angle += .01f;
			if (engine.state.angle > 1) {
				engine.state.angle = 0;
			}

			// Drawing is throttled to the screen update rate, so there
			// is no need to do timing here.
			engine_draw_frame(&engine);
		}
	}
}
