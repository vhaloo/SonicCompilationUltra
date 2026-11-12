#include "motion_controls.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <initializer_list>
using namespace scu::motion;
static void check(bool ok,const char *text){if(!ok){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
static Quaternion pose(float x,float y){const float a=x*.00872664626f,b=y*.00872664626f;
    return {std::sin(a)*std::cos(b),std::cos(a)*std::sin(b),-std::sin(a)*std::sin(b),std::cos(a)*std::cos(b)};}
int main(){
    unsigned cases=0;
    for(int rotation=0;rotation<4;++rotation)for(int sensitivity:{40,100,240})for(int dead:{5,20,50}){
        Controller c;Options o;o.mode=2;o.sensitivity=sensitivity;o.deadzone=dead;c.configure(o);
        Sample s;s.source=1;s.rotation=rotation;s.milliseconds=1000;s.orientation=pose(0,40);c.update(s);
        check(c.state(1000).ready&&c.state(1000).directions==0,"non-flat neutral calibration");
        for(int i=1;i<20;++i){s.milliseconds=1000+i*20;s.orientation=pose(0,75);c.update(s);}
        auto state=c.state(s.milliseconds);check(state.directions!=0,"relative tilt ignored");
        const uint16_t expected[]={1u<<7,1u<<5,1u<<6,1u<<4};
        check(state.directions==expected[rotation],"display rotation mapped to the wrong direction");
        check(!(state.directions&(1u<<6))||!(state.directions&(1u<<7)),"opposite horizontal directions");
        check(!(state.directions&(1u<<4))||!(state.directions&(1u<<5)),"opposite vertical directions");
        const auto before=state.directions;s.orientation.x=-s.orientation.x;s.orientation.y=-s.orientation.y;
        s.orientation.z=-s.orientation.z;s.orientation.w=-s.orientation.w;s.milliseconds+=20;c.update(s);
        check(c.state(s.milliseconds).directions==before,"quaternion sign flipped control");
        check(!c.state(s.milliseconds+251).ready,"stale sensor remained held");
        c.recenter();c.update(s);check(c.state(s.milliseconds).directions==0,"recenter failed");++cases;
    }
    for(int source:{2,3}){
        Controller c;Options o;o.mode=1;c.configure(o);Sample s;s.source=source;s.gravity={0,0,9.81f};s.milliseconds=1000;c.update(s);
        for(int i=1;i<20;++i){s.gravity={-5,0,8.5f};s.milliseconds+=20;c.update(s);}
        check(c.state(s.milliseconds).directions==(1u<<7),"gravity fallback direction");
        s.gravity.x=std::numeric_limits<float>::quiet_NaN();s.milliseconds+=20;c.update(s);
        check(!c.state(s.milliseconds).ready,"invalid gravity remained active");++cases;
    }
    Controller c;Options o;o.mode=1;o.jumpGesture=2;c.configure(o);Sample s;s.source=1;s.milliseconds=1000;c.update(s);
    for(int i=0;i<20;++i){s.milliseconds+=20;c.update(s);}
    s.acceleration={0,0,12};s.milliseconds+=20;c.update(s);
    check(c.state(s.milliseconds).jump,"flick not detected");
    for(int i=0;i<20;++i){s.milliseconds+=20;c.update(s);}
    check(!c.state(s.milliseconds).jump,"long shake retriggered");
    s.acceleration={};s.milliseconds+=20;c.update(s);s.milliseconds+=20;c.update(s);
    s.acceleration={12,0,0};s.milliseconds+=20;c.update(s);
    check(c.state(s.milliseconds).jump,"released gesture did not rearm");
    o.mode=0;c.configure(o);check(!c.state(s.milliseconds).jump,"off did not release gesture");
    // The same upward movement is expressed in each device posture's axes.
    // A modest 2 m/s2 lift must work; a sideways shake or braking a descent must not.
    for(int rotation=0;rotation<4;++rotation)for(auto orientation:{pose(0,0),pose(40,25),pose(-30,-50)}) {
        Controller lift;Options options;options.mode=1;options.jumpGesture=1;lift.configure(options);
        Sample sample;sample.source=1;sample.orientation=orientation;sample.rotation=rotation;sample.milliseconds=1000;
        for(int i=0;i<25;++i){sample.milliseconds+=20;lift.update(sample);}
        const Vec upVector{2*(orientation.x*orientation.z-orientation.w*orientation.y),
            2*(orientation.y*orientation.z+orientation.w*orientation.x),1-2*(orientation.x*orientation.x+orientation.y*orientation.y)};
        for(int i=0;i<6;++i){sample.milliseconds+=20;sample.acceleration={2*upVector.x,2*upVector.y,2*upVector.z};lift.update(sample);}
        check(lift.jumpSequence()==1,"gentle world-up wrist lift missed");
        for(int i=0;i<30;++i){sample.milliseconds+=20;sample.acceleration={};lift.update(sample);}
        check(lift.jumpSequence()==1,"resting raised wrist repeated Jump");
        for(int i=0;i<6;++i){sample.milliseconds+=20;sample.acceleration={2*upVector.x,2*upVector.y,2*upVector.z};lift.update(sample);}
        check(lift.jumpSequence()==2,"second wrist lift did not rearm");
        lift.recenter();sample.acceleration={};
        for(int i=0;i<25;++i){sample.milliseconds+=20;lift.update(sample);}
        const auto before=lift.jumpSequence();
        for(int sign:{-1,1})for(int i=0;i<8;++i){sample.milliseconds+=20;sample.acceleration={sign*2*upVector.x,sign*2*upVector.y,sign*2*upVector.z};lift.update(sample);}
        check(lift.jumpSequence()==before,"braking a lowered wrist triggered Jump");
    }
    Controller angular;Options angularOptions;angularOptions.mode=1;angular.configure(angularOptions);
    Sample poseSample;poseSample.source=1;poseSample.milliseconds=1000;
    for(int i=0;i<25;++i){poseSample.milliseconds+=20;angular.update(poseSample);}
    for(int i=0;i<8;++i){poseSample.milliseconds+=20;poseSample.orientation=pose(-i*2.f,0);angular.update(poseSample);}
    check(angular.jumpSequence()==1,"quick upward wrist tilt missed");
    Controller delayed;Options delayedOptions;delayedOptions.mode=1;delayed.configure(delayedOptions);
    Sample stream;stream.source=1;stream.milliseconds=1000;
    for(int i=0;i<25;++i){stream.milliseconds+=20;delayed.update(stream);auto old=stream;old.milliseconds-=2;delayed.update(old);}
    for(int i=0;i<6;++i){stream.milliseconds+=20;stream.acceleration={0,0,2};delayed.update(stream);auto old=stream;old.milliseconds-=2;delayed.update(old);}
    check(delayed.jumpSequence()==1,"out-of-order sensor callbacks repeatedly recalibrated the lift");
    constexpr uint16_t left=1u<<6,right=1u<<7,up=1u<<4,down=1u<<5,jump=1u<<1;
    check(combine(left|down,right|jump)==(right|down|jump),"touch horizontal priority lost other axis");
    check(combine(left|down,up)==(left|up),"gamepad vertical priority failed");
    std::printf("PASS %u rotation/sensitivity/deadzone and fallback cases; calibration, stale/invalid input, flick rearm and direct-control priority. No hardware gesture claim.\n",cases);
}
