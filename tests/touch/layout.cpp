#include "touch_controls.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
using namespace scu::touch;
static void require(bool v, const char *message) {
    if (!v) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}
static bool inside(Rect r, Rect s) {
    return r.w > 0 && r.h > 0 && r.x >= s.x - .01f && r.y >= s.y - .01f &&
           r.x + r.w <= s.x + s.w + .01f && r.y + r.h <= s.y + s.h + .01f;
}
static Point center(Rect r) {
    return {r.x + r.w / 2, r.y + r.h / 2};
}
static Point button(const Layout &l, Action a) {
    for (auto &b : l.buttons)
        if (b.action == a)
            return center(b.rect);
    require(false, "missing button");
    return {};
}
int main() {
    int cases = 0;
    for (auto size : {Point{240, 240}, Point{320, 568}, Point{568, 320}, Point{412, 915},
                      Point{915, 412}, Point{600, 960}, Point{960, 600}, Point{800, 1280},
                      Point{1600, 900}, Point{1440, 420}, Point{280, 600}, Point{1200, 500}})
        for (bool menu : {false, true})
            for (bool left : {false, true})
                for (bool controls : {false, true})
                    for (int mode : {0, 1, 2})
                        for (float scale : {.65f, 1.f, 1.6f}) {
                            Options o;
                            o.menu = menu;
                            o.leftHanded = left;
                            o.controls = controls;
                            o.layout = mode;
                            o.scale = scale;
                            Rect safe{24, 32, size.x - 48, size.y - 64};
                            auto l = arrange(safe, o);
                            require(inside(l.canvas, safe), "canvas outside safe area");
                            if (l.stick.w)
                                require(inside(l.stick, safe), "stick outside safe area");
                            for (auto &b : l.buttons)
                                require(inside(b.rect, safe), "button outside safe area");
                            if (menu)
                                require(l.buttons.size() == 5, "menu dock is not complete");
                            else {
                                require(button(l, Action::Menu).x >= safe.x, "menu unavailable");
                                if (controls)
                                    require(l.stick.w > 0, "missing stick");
                                else
                                    require(l.stick.w == 0, "hidden stick still has a hit target");
                                if (mode == 0 && controls)
                                    require(l.deck == (safe.h > safe.w * 1.1f),
                                            "auto portrait split");
                            }
                            ++cases;
                        }
    auto l = arrange({0, 0, 412, 915}, {});
    Input in;
    auto stick = center(l.stick);
    stick.x += l.stick.w * .45f;
    require(in.down(1, stick, l) == Action::Stick, "stick capture");
    require(in.down(2, center(l.stick), l) == Action::None, "second stick capture stole first");
    in.down(3, button(l, Action::B), l);
    in.down(4, button(l, Action::Rewind), l);
    auto s = in.state(l, .25f);
    require((s.buttons & (1 << 7)) && (s.buttons & 1) && s.rewind, "simultaneous move/jump/rewind");
    in.up(3);
    s = in.state(l, .25f);
    require(!(s.buttons & 1) && s.rewind, "finger release changed another finger");
    in.move(4, {-100, -100});
    require(!in.state(l, .25f).rewind, "sliding off rewind kept it pressed");
    in.move(1, {-1000, -1000});
    s = in.state(l, .25f);
    require(std::abs(s.stick.x) <= 1 && std::abs(s.stick.y) <= 1, "stick not clamped");
    in.clear();
    s = in.state(l, .25f);
    require(s.buttons == 0 && !s.rewind, "rotation/focus clear stuck input");
    in.down(5, button(l, Action::A), l);
    in.down(6, button(l, Action::B), l);
    require((in.state(l, .25f).buttons & 3) == 3, "Spinball left+right");
    in.up(5);
    require(in.state(l, .25f).buttons == 1, "left release affected right flipper");
    Options menu;
    menu.menu = true;
    auto m = arrange({0, 0, 320, 568}, menu);
    in.clear();
    in.down(9, button(m, Action::ScrollDown), m);
    require(in.state(m, .25f).scroll == 1, "held scroll down");
    in.up(9);
    require(in.state(m, .25f).scroll == 0, "scroll release");
    for (auto id : {"sonic1","sonic2","sonic3","sandk","sonic3k","sonic2k","sphere"}) {
        require(profileForGame(id)==Profile::Jump,"platformer did not get one jump action");
        for(bool watch:{false,true}) {
            Options o;o.watch=watch;o.circular=watch;o.profile=profileForGame(id);
            auto layout=arrange({0,0,438,438},o);
            require(std::count_if(layout.buttons.begin(),layout.buttons.end(),[](auto b){
                return b.action==Action::A||b.action==Action::B||b.action==Action::C;})==1,
                "platformer has duplicate action buttons");
        }
    }
    require(profileForGame("compilation",1)==Profile::Jump&&
            profileForGame("compilation",2)==Profile::Jump&&
            profileForGame("compilation",0)==Profile::TwoActions,"Classics bank profile");
    for(bool watch:{false,true})for(bool left:{false,true}) {
        Options o;o.watch=watch;o.circular=watch;o.leftHanded=left;o.profile=Profile::Pinball;
        auto layout=arrange({0,0,438,438},o);
        require(button(layout,Action::A).x<button(layout,Action::B).x,"flipper sides swapped");
        Input controls;
        require(controls.down(10,button(layout,Action::A),layout)==Action::A,"left flipper obscured");
        require(controls.down(11,button(layout,Action::B),layout)==Action::B,"right flipper obscured");
        require((controls.state(layout,.25f).buttons&3)==3,"simultaneous flippers failed");
        controls.up(10);require(controls.state(layout,.25f).buttons==1,"left release affected right");
        controls.move(11,{0,0});require(controls.state(layout,.25f).buttons==0,"flipper stuck outside circle");
        auto target=std::find_if(layout.buttons.begin(),layout.buttons.end(),[](auto b){return b.action==Action::B;});
        require(controls.down(12,{target->rect.x,target->rect.y},layout)==Action::None,
                "invisible circular-button corner captures touches");
    }
    Options jump;jump.watch=jump.circular=true;jump.profile=Profile::Jump;
    auto jumpLayout=arrange({0,0,438,438},jump);
    Input quick;
    quick.down(50,button(jumpLayout,Action::A),jumpLayout,1000);quick.up(50);
    require(quick.state(jumpLayout,.25f,1033).buttons==(1u<<1),"between-frame Jump tap was dropped or maps to six-button Y");
    require(quick.state(jumpLayout,.25f,1100).buttons==0,"minimum tap pulse did not release");
    quick.down(51,button(jumpLayout,Action::A),jumpLayout,1200);
    quick.move(51,{0,0});
    require(quick.state(jumpLayout,.25f,1400).buttons==(1u<<1),"Jump lost pointer capture on thumb drift");
    quick.up(51);require(quick.state(jumpLayout,.25f,1401).buttons==0,"captured Jump did not release");
    quick.down(52,button(jumpLayout,Action::A),jumpLayout,1500);quick.clear();
    require(quick.state(jumpLayout,.25f,1501).buttons==0,"focus/menu clear retained a tap");
    SwipeJump swipe;
    swipe.down(1,{100,100},1000,24);swipe.move(1,{102,70},1060);
    require(swipe.pressed(1090),"upward swipe failed");
    swipe.move(1,{103,20},1200);require(!swipe.pressed(1210),"same finger retriggered swipe");
    swipe.up(1);swipe.down(1,{100,100},1300);swipe.move(1,{100,70},1360);
    require(swipe.pressed(1370),"new swipe did not rearm");
    swipe.clear();swipe.down(2,{100,100},2000);swipe.move(2,{140,101},2060);
    require(!swipe.pressed(2061),"horizontal drag triggered Jump");
    swipe.move(2,{100,130},2100);require(!swipe.pressed(2101),"downward drag triggered Jump");
    swipe.clear();swipe.down(3,{100,100},3000);swipe.move(3,{100,50},3800);
    require(!swipe.pressed(3801),"slow drift triggered Jump");
    std::printf("PASS %d adaptive layouts plus multitouch ownership, native A/B/C mapping, quick-tap retention, swipe Jump, simultaneous flippers, "
                "rewind, release and scrolling. No game linked or executed.\n",
                cases);
}
