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
    Controller c;Options o;o.mode=1;o.flick=true;c.configure(o);Sample s;s.source=1;s.milliseconds=1000;c.update(s);
    for(int i=0;i<20;++i){s.milliseconds+=20;c.update(s);}
    s.acceleration={0,0,12};s.milliseconds+=20;c.update(s);
    check(c.state(s.milliseconds).flick,"flick not detected");
    for(int i=0;i<20;++i){s.milliseconds+=20;c.update(s);}
    check(!c.state(s.milliseconds).flick,"long shake retriggered");
    s.acceleration={};s.milliseconds+=20;c.update(s);s.milliseconds+=20;c.update(s);
    s.acceleration={12,0,0};s.milliseconds+=20;c.update(s);
    check(c.state(s.milliseconds).flick,"released gesture did not rearm");
    o.mode=0;c.configure(o);check(!c.state(s.milliseconds).flick,"off did not release gesture");
    constexpr uint16_t left=1u<<6,right=1u<<7,up=1u<<4,down=1u<<5,jump=1u<<9;
    check(combine(left|down,right|jump)==(right|down|jump),"touch horizontal priority lost other axis");
    check(combine(left|down,up)==(left|up),"gamepad vertical priority failed");
    std::printf("PASS %u rotation/sensitivity/deadzone and fallback cases; calibration, stale/invalid input, flick rearm and direct-control priority. No hardware gesture claim.\n",cases);
}
