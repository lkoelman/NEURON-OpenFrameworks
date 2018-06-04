#pragma once

#include "ofMain.h"
#include "zmq.hpp"

// A signal sample. The signal samples are contained in GraphedVar.samples.
// The sample time and value are denoted as t, v and the screen positions
// as x, y.
typedef struct GraphedVar
{
	// Variable metadata
	string name;
	unsigned int var_id;
	// Screen position related
	ofPoint ax_origin;
	float y_height;
	float y_per_v;
	float x_per_t;
	// Limits of graphed value
	float v_lim_upper;
	float v_lim_lower;
	float t_lim_lower; // constantly updated
	std::deque<ofPoint> *samples;

	// GraphedVar(string name, int id) :
	// 	name(name),
	// 	var_id(id),
	// 	ax_origin(ofPoint(0.0)),
	// 	y_height(100.0),
	// 	y_per_v(1.0),
	// 	x_per_t(0.5),
	// 	v_lim_upper(40.0),
	// 	v_lim_lower(-80.0),
	// 	t_lim_lower(0.0),
	// 	samples({{0.0, 0.0, 0.0}});

} GraphedVar;

class ofApp : public ofBaseApp
{

  public:
	ofApp(string config_path) :
		config_file(config_path)
		{}

	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	// Supporting methods
	void add_graphed_var(string var_name, ofPoint ax_origin);
	int _setup_socket(string protocol, string host, unsigned int port);

	inline static void sample_to_screen(
		const GraphedVar &var,
		const ofPoint &sample,
		ofPoint &point);

	// We need one polyline per graphed variable
	// vector<GraphedVar> graphed_vars;
	map<int, GraphedVar> graphed_vars;
	string config_file;

  private:

	// Socket communication
	std::unique_ptr<zmq::context_t> _p_context;
	// zmq::context_t& _get_context();

	std::unique_ptr<zmq::socket_t> _p_socket;
	// zmq::socket_t& _get_socket();
	// zmq::context_t context;
    // zmq::socket_t subscriber;
};
