#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "zmq.hpp"

#define DEBUG 1
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef DEBUG
#define DBGMSG(os, msg) \
  (os) << "[DEBUG::" << __FILENAME__ << ":" << __LINE__ << "] " \
       << msg << std::endl
#else
#define DBGMSG(os, msg)
#endif


class GraphedVariable;

/**
 * Our OpenFrameworks applications where all the magic happens.
 */
class ofApp : public ofBaseApp, public ofxMidiListener
{

  public:
    ofApp(std::string config_path) : 
        config_file(config_path),
        LOG_PREFIX("\n[NeuroControl]: ") {}

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
        const GraphedVariable &var,
        const ofPoint &sample,
        ofPoint &point);

    string config_file;

    // We need one polyline per graphed variable
    // vector<GraphedVar> variables;
    map<unsigned int, std::shared_ptr<GraphedVariable>> variables;
    float max_time;     // maximum timepoint received for any variable

    // MIDI communication
    ofxMidiIn midiIn;
    ofxMidiMessage midiMessage;
    void newMidiMessage(ofxMidiMessage& eventArgs);

  private:

    // Socket communication
    std::unique_ptr<zmq::context_t> _p_context;
    // zmq::context_t& _get_context();

    std::unique_ptr<zmq::socket_t> _p_socket;
    // zmq::socket_t& _get_socket();
    // zmq::context_t context;
    // zmq::socket_t subscriber;

    const std::string LOG_PREFIX;
};


typedef struct {
    double gid;
    double t;
    double v;
} sample_t;


// A graphed variable. 
// Its responsibilities are data storage for the incoming signal samples
// and storing plotting options.
// The signal samples are contained in GraphedVar.samples.
// The sample time and value are denoted as t, v and the screen positions
// as x, y.
class GraphedVariable
{
  public:
    GraphedVariable(unsigned int id, std::string varname) :
        name(varname),
        id(id),
        ax_origin(0.0, 0.0),
        y_height(50.0),
        y_per_v(1.0),
        x_per_t(0.5),
        x_width_max(500.0),
        v_lim_upper(40.0),
        v_lim_lower(-80.0),
        t_lim_lower(0.0)
    {
        // samples = new std::deque<ofPoint>({ofPoint(0.0)});
        samples = std::unique_ptr<std::deque<ofPoint>>(
                 new std::deque<ofPoint>({ofPoint(0.0, 0.0)}));
    }

    int y_height_max() const {
        return (v_lim_upper - v_lim_lower) * y_per_v;
    }

    // std::deque<ofPoint> * samples;
    std::unique_ptr<std::deque<ofPoint>> samples;

    // Variable metadata
    string name;
    unsigned int id;

    // Screen position related
    ofPoint ax_origin;

    // Size and dimensions
    float y_height;
    float y_per_v;
    float x_per_t;

    float x_width_max; // max pixel width of all graphs showing t

    // Limits of graphed value
    float v_lim_upper;
    float v_lim_lower;
    float t_lim_lower; // constantly updated
};