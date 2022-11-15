#pragma once
#include "Document.h"
#include "../Graphs/OpenGL/GL_RM.h"
#include "../Graphs/OpenGL/GL_StringCache.h"
#include "../Engine/Namespace/Parameter.h"
#include <map>
#include <SDL_events.h>

class PlotWindow : public Document
{
public:
	PlotWindow(SDL_Window* window, GL_Context &context);
	virtual ~PlotWindow();
	void load(const std::string &path) override { Document::load(path); redraw(); }

	bool   animating() const{ return tnf > 0.0; }
	double next_frame_schedule() const{ return tnf; }
	void   animate(double now);
	bool   needs_redraw() const{ return need_redraw; }

	operator bool() const{ return !closed; }

	void draw();
	//void redraw(){ need_redraw = true; } --> moved to base class

	void reshape(int w, int h);
	bool handle_event(const SDL_Event &event);
	bool handle_key(SDL_Keysym key, bool release);

	int  accum_size() const { return accum; }
	
	void start_animations(); // call after Parameter::anim_start()
protected:
	void stop_animations();

	SDL_Window *window;
	int         w, h;
	int         accum;      // accumulation buffer size
	bool        closed;     // should window close?
   
	double      tnf;        // scheduled time for next frame
	double      last_frame; // time of last draw
	GL_RM       rm;

	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	std::set<SDL_Keycode> keys; // pressed keys

	void move(double dx, double dy, double dz, bool kbd, int buttons=0);
	enum Zoom{ Axis, Camera, Inrange }; 
	void zoom(double d, Zoom what, int mx = -1, int my = -1);
	void change_parameter(int i, cnum delta);

	void status(); // draw status bar
	mutable GL_StringCache labelCache;
};

