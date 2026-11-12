#include "motion_controls.h"
#include <algorithm>
#include <cmath>
namespace scu::motion {
static float dot(Vec a, Vec b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static bool normalize(Vec &v) {
    const float n = std::sqrt(dot(v,v));
    if (!std::isfinite(n) || n < .01f) return false;
    v.x /= n; v.y /= n; v.z /= n; return true;
}
static bool normalize(Quaternion &q) {
    const float n = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    if (!std::isfinite(n) || n < .01f) return false;
    q.x/=n; q.y/=n; q.z/=n; q.w/=n; return true;
}
void Controller::configure(Options value) {
    value.mode=std::clamp(value.mode,0,2);
    value.sensitivity=std::clamp(value.sensitivity,40,240);
    value.deadzone=std::clamp(value.deadzone,5,50);
    value.flickThreshold=std::clamp(value.flickThreshold,3,20);
    if (value.mode != options.mode) recenter();
    options=value;
}
void Controller::recenter() {
    calibrated=false; lastSample=calibratedAt=lastFlick=pulseUntil=0;
    filteredX=filteredY=0; gestureArmed=true; current={};
}
void Controller::update(const Sample &s) {
    if (!options.mode || !s.source || !s.milliseconds) { recenter(); return; }
    Quaternion q=s.orientation;
    Vec g=s.gravity;
    if ((s.source==1 && !normalize(q)) || (s.source!=1 && !normalize(g))) {
        recenter(); return;
    }
    if (s.milliseconds==lastSample) return;
    if (s.milliseconds<lastSample || (lastSample && s.milliseconds-lastSample>250) ||
        s.source!=source || s.rotation!=rotation) recenter();
    if (!calibrated) {
        neutral=q; neutralGravity=g; source=s.source; rotation=s.rotation;
        calibratedAt=s.milliseconds; calibrated=true;
    }
    float x=0,y=0;
    if (s.source==1) {
        // Relative orientation in the neutral device's axes; quaternion sign
        // changes represent the same pose and must never reverse the controls.
        Quaternion d{neutral.w*q.x-neutral.x*q.w-neutral.y*q.z+neutral.z*q.y,
                     neutral.w*q.y+neutral.x*q.z-neutral.y*q.w-neutral.z*q.x,
                     neutral.w*q.z-neutral.x*q.y+neutral.y*q.x-neutral.z*q.w,
                     neutral.w*q.w+neutral.x*q.x+neutral.y*q.y+neutral.z*q.z};
        if(d.w<0){d.x=-d.x;d.y=-d.y;d.z=-d.z;d.w=-d.w;}
        const float n=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);
        const float angle=n>.00001f?2*std::atan2(n,d.w)/n:2.f;
        x=d.y*angle; y=d.x*angle;
    } else {
        const float alignment=std::clamp(dot(g,neutralGravity),-1.f,1.f);
        x=std::atan2(g.z*neutralGravity.x-g.x*neutralGravity.z,alignment);
        y=std::atan2(g.y*neutralGravity.z-g.z*neutralGravity.y,alignment);
    }
    for (int i=0;i<(rotation&3);++i) { const float old=x; x=-y; y=old; }
    const float gain=options.sensitivity/100.f/.436332313f;
    x=std::clamp(x*gain,-1.f,1.f); y=std::clamp(y*gain,-1.f,1.f);
    if(options.invertX)x=-x;
    if(options.invertY)y=-y;
    const float dt=lastSample?std::clamp(float(s.milliseconds-lastSample)*.001f,.001f,.1f):.02f;
    const float blend=1-std::exp(-dt/.045f);
    filteredX+=(x-filteredX)*blend; filteredY+=(y-filteredY)*blend;
    auto axis=[&](float v){const float dead=options.deadzone*.01f;
        return std::copysign(std::max(0.f,std::abs(v)-dead)/(1-dead),v);};
    current.x=axis(filteredX); current.y=options.mode==2?axis(filteredY):0;
    current.directions=0;
    auto direction=[&](float v,int negative,int positive){
        if(std::abs(v)>.05f)current.directions|=uint16_t(1u<<(v<0?negative:positive));};
    direction(current.x,6,7); direction(current.y,4,5);
    const float force=std::sqrt(dot(s.acceleration,s.acceleration));
    if(std::isfinite(force)) {
        if(force<options.flickThreshold*.5f)gestureArmed=true;
        if(options.flick && gestureArmed && force>options.flickThreshold &&
           s.milliseconds-calibratedAt>300 && (!lastFlick || s.milliseconds-lastFlick>450)) {
            pulseUntil=s.milliseconds+120;lastFlick=s.milliseconds;gestureArmed=false;
        }
    }
    current.ready=true; lastSample=s.milliseconds;
}
State Controller::state(uint64_t now) const {
    if(!options.mode || !lastSample || now<lastSample || now-lastSample>250)return {};
    State result=current; result.flick=options.flick && now<pulseUntil; return result;
}
uint16_t combine(uint16_t sensor,uint16_t direct) {
    constexpr uint16_t horizontal=(1u<<6)|(1u<<7),vertical=(1u<<4)|(1u<<5);
    if(direct&horizontal)sensor&=~horizontal;
    if(direct&vertical)sensor&=~vertical;
    return sensor|direct;
}
} // namespace scu::motion
