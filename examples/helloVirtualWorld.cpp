/*
What is our "Hello world!" app?

An agent orbits around the origin emitting the audio line input. The camera
view can be switched between a freely navigable keyboard/mouse controlled mode 
and a sphere follow mode.

Requirements:
2 channels of spatial sound
2 windows, one front view, one back view
stereographic rendering
*/

#include "allocore/al_Allocore.hpp"
#include "alloutil/al_ControlNav.hpp"
using namespace al;

struct Agent : public SoundSource, public Drawable{

	Agent()
	: oscPhase(0), oscEnv(1)
	{
		pos(-4, 0, -4);
	}

	virtual ~Agent(){}

	virtual void onProcess(AudioIOData& io){
		while(io()){
			//float s = io.in(0);
			//float s = rnd::uniform(); // make noise, just to hear something
			//float s = sin(oscPhase * M_2PI);
			float s = al::rnd::uniformS();
			
			s *= (oscEnv*=0.999);
			
			if(oscEnv < 0.00001){ oscEnv=1; oscPhase=0; }
			
			//float s = phase * 2 - 1;
			oscPhase += 440./io.framesPerSecond();
			if(oscPhase >= 1) oscPhase -= 1;
			writeSample(s*4.);
		}
	}
	
	virtual void onUpdateNav(){
		smooth(0.9);
		//if((phase+=0.01) >= M_2PI) phase -= M_2PI;
		//pos(cos(phase), sin(phase), 0);
		//pos(0,0,0);
		
		//spin(0.3, 0, 0);
		//moveF(0.04);
		step();
	}
	
	virtual void onDraw(Graphics& g){
		g.pushMatrix();
		g.scale(10);
		g.begin(g.LINES);
			g.color(1, 1, 1);
			for (int x=-1; x<=1; x+=2) {
			for (int y=-1; y<=1; y+=2) {
			for (int z=-1; z<=1; z+=2) {
				g.vertex(x,y,z);
			}}} 
			for (int z=-1; z<=1; z+=2) {
			for (int x=-1; x<=1; x+=2) {
			for (int y=-1; y<=1; y+=2) {
				g.vertex(x,y,z);
			}}}
			for (int z=-1; z<=1; z+=2) {
			for (int y=-1; y<=1; y+=2) {
			for (int x=-1; x<=1; x+=2) {
				g.vertex(x,y,z);
			}}}
		g.end();
		g.popMatrix();

		g.pushMatrix();
		g.multMatrix(matrix());
	
		g.begin(g.TRIANGLES);
			float ds = 0.5;
			
			g.color(1,1,1);
			g.vertex(    0, 0, ds*2);
			g.color(1,1,0);
			g.vertex( ds/2, 0,-ds);
			g.color(1,0,1);
			g.vertex(-ds/2, 0,-ds);
			
			g.color(1,1,1);
			g.vertex(    0, 0, ds*2);
			g.color(0,1,1);
			g.vertex( 0, ds/2,-ds);
			g.color(1,0,1);
			g.vertex(-ds/2, 0,-ds);
			
			g.color(1,1,1);
			g.vertex(    0, 0, ds*2);
			g.color(1,1,0);
			g.vertex( ds/2, 0,-ds);
			g.color(0,1,1);
			g.vertex( 0, ds/2, -ds);
			
		g.end();
		
		g.popMatrix();
	}
	
	double oscPhase, oscEnv;
};


Listener * listener;
Nav navMaster(Vec3d(0,0,-4), 0.95);
std::vector<Agent> agents(1);
Stereographic stereo;
#define AUDIO_BLOCK_SIZE 256

#ifdef ALLOSPHERE
AudioScene scene(3, 3, AUDIO_BLOCK_SIZE);
const int numSpeakers = 16;
const double topAz = 90;
const double topEl = 45;
const double midAz = (360./8);
const double midEl = 0;
const double botAz = 90;
const double botEl =-45;
AmbiDecode::Speaker speakers[numSpeakers] = {
	{ 0.5*midAz, midEl,  3-1},
	{ 1.5*midAz, midEl,  6-1},
	{ 2.5*midAz, midEl, 17-1},
	{ 3.5*midAz, midEl, 18-1},
	{-0.5*midAz, midEl,  4-1},
	{-1.5*midAz, midEl,  5-1},
	{-2.5*midAz, midEl, 20-1},
	{-3.5*midAz, midEl, 19-1},
	
	{ 0.5*topAz, topEl,  7-1},
	{ 1.5*topAz, topEl,  9-1},
	{-0.5*topAz, topEl,  8-1},
	{-1.5*topAz, topEl, 21-1},

	{ 0.5*botAz, botEl, 24-1},
	{ 1.5*botAz, botEl, 23-1},
	{-0.5*botAz, botEl, 10-1},
	{-1.5*botAz, botEl, 22-1},
};
#else
AudioScene scene(2, 1, AUDIO_BLOCK_SIZE);
const int numSpeakers = 2;
AmbiDecode::Speaker speakers[] = {
	{  45, 0,  0},
	{ -45, 0,  1},
};
#endif


void audioCB(AudioIOData& io){
	int numFrames = io.framesPerBuffer();

	for(unsigned i=0; i<agents.size(); ++i){
		io.frame(0);
		agents[i].onUpdateNav();
		agents[i].onProcess(io);
	}

	navMaster.step(0.5);
	listener->pos(navMaster.pos());
	listener->quat(navMaster.quat());

	scene.encode(numFrames, io.framesPerSecond());
	scene.render(&io.out(0,0), numFrames);
	
	//printf("%g\n", io.out(0,0));
}


struct MyWindow : public Window, public Drawable{
	
	MyWindow() {}

	bool onFrame(){

		Pose pose(navMaster * transform);


		Viewport vp(dimensions().w, dimensions().h);
		stereo.draw(gl, cam, pose, vp, *this);

//		printf("pos %f %f %f\n", navMaster.pos()[0], navMaster.pos()[1], navMaster.pos()[2]);		
		
		return true;
	}
	
	bool onKeyDown(const Keyboard& k) {
		
		if (k.key() == Key::Tab) {
			stereo.stereo(!stereo.stereo());
		}
		
		return true;
	}
	
	virtual void onDraw(Graphics& g){

		for(unsigned i=0; i<agents.size(); ++i){
			agents[i].onDraw(g);
		}
	}

	GraphicsGL gl;
	Pose transform;
	Camera cam;
};


int main (int argc, char * argv[]){

	listener = &scene.createListener(2);
	
//	listener->speakerPos(0,0,-60);
//	listener->speakerPos(1,1, 60);
////	listener->speakerPos(0,0,-45);
////	listener->speakerPos(1,1, 45);
//
	


	
	listener->numSpeakers(numSpeakers);
	for(int i=0; i<numSpeakers; ++i){
		listener->speakerPos(
			i,
			speakers[i].deviceChannel,
			speakers[i].azimuth,
			speakers[i].elevation
		);
	}
	
	for(unsigned i=0; i<agents.size(); ++i) scene.addSource(agents[i]);

	MyWindow windows[6];
	
	windows[5].create(Window::Dim(200, 500, 200,200), "Bottom");
	windows[0].create(Window::Dim(  0, 300, 200,200), "Left");
	windows[1].create(Window::Dim(200, 300, 200,200), "Center");
	windows[2].create(Window::Dim(400, 300, 200,200), "Right");
	windows[3].create(Window::Dim(600, 300, 200,200), "Back");
	windows[4].create(Window::Dim(200, 100, 200,200), "Top");

	for(int i=0; i<4; ++i){
		windows[i].add(new StandardWindowKeyControls);
		windows[i].add(new NavInputControl(&navMaster));
		windows[i].transform.quat().fromAxisAngle(-90 + i*90, 0, 1, 0);
		windows[i].cam.fovy(90);
	}
	for(int i=4; i<6; ++i){
		windows[i].add(new StandardWindowKeyControls);
		windows[i].add(new NavInputControl(&navMaster));
		windows[i].transform.quat().fromAxisAngle(-90 + i*180, 1, 0, 0);
		windows[i].cam.fovy(90);
	}

	AudioIO audioIO(AUDIO_BLOCK_SIZE, 44100, audioCB, NULL, numSpeakers, 2);
	audioIO.start();

	MainLoop::start();
	return 0;
}
