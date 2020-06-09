#include <fmt/format.h>
#include "nana/gui/detail/general_events.hpp"
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/paint/image.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/deploy.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui/filebox.hpp>
#include <iostream>
#include <nana/gui/timer.hpp>
#include <nana/gui/tooltip.hpp>
#include <nana/gui/drawing.hpp>
#include <nana/gui/widgets/slider.hpp>

using namespace nana;

class fm : public form
{
  place plc{*this};
  timer tmr;
  group mn{*this, "", true},
      bttns{mn, ""},        //field for control buttons
      submn{mn, ""};        //field for equalizer, buttons for changing volume level
  button b_pl{bttns, ("")}, //play-pause
      b_s{bttns, ("")},     //stop
      b_n{bttns, ("")},     //next
      b_pr{bttns, ("")},    //previous
      b_rpl{bttns, ("")},   //replay
      b_vmin{submn, ("")},
      b_eq{submn, ("")},
      b_vmax{submn, ("")};
  listbox lbx{*this};
  menubar mnbr{*this};
  slider sldr{submn}; //progress prg{submn};
  /* класс для более удобного оформления элементов окна, которые собраны в группы(такие как кнопки и кнопки с
                * прогресс-баром), либо описаны самостоятельными
                * в public описываются параметры поля окна, его подпространства, в private - функции без
                  возвращаемых значений, осуществляющих работу с элементами класса                                                                                     */
public:
  fm()
      : form(API::make_center(300, 600), appear::decorate<appear::minimize, appear::maximize, appear::sizable>()) //300,600
  {
    nana::API::track_window_size(*this, {300, 600}, false);
    nana::API::track_window_size(*this, {300, 600}, true);
    plc.div("vert <menubar weight=28><main weight=30%><listbox>");
    plc["menubar"] << mnbr;
    plc["main"] << mn;
    mn.div("<vert all min=260 gap=10 margin=10>"); //weight=50% gap=5 margin=10><weight=30%
    mn["all"] << bttns << submn;
    plc.field("listbox") << lbx;

    lbx.append_header("Songs' Headers");
    lbx.events().selected([](const arg_listbox &arg) {
      static auto ptrr = arg;
      //return int a;       //the length of a song
    });

    m_init_buttons();
    // m_init_listbox();
    m_make_menus();
    m_init_submain();

    this->events().unload([this](const arg_unload &ei) { // yes/no messagebox that opens when you try to exit the programme
      msgbox mb(*this, ("Question"), msgbox::yes_no);
      mb.icon(mb.icon_question) << ("Are you sure?");
      ei.cancel = (mb() == mb.pick_no);
    });
    plc.collocate();

    //fm icon(*this, paint::image("../media/player.bmp"));

    /* the method doesnt get applied because of unknown mistake - it claims nana::paint has no ".paste()
   * option, what leads to a problem"*/
  };

private:
  std::filesystem::path m_pick_file(bool is_open) const
  {
    filebox fbox(*this, is_open);
    fbox.add_filter("MP3", "*.mp3");
    fbox.add_filter("All Files", "*.*");
    auto files = fbox.show();
    return (files.empty() ? std::filesystem::path{} : files.front());
  }
  /*function that helps find the files with mp3 extension in the directory
   *  of computer and return its path  */
  void m_init_buttons()
  {
    bttns.div("buttons gap=15 margin=[5,10]"); //grid=[3,3] collapse(2,2,3,2)
    bttns["buttons"] << b_s << b_pr << b_pl << b_n << b_rpl;
    msgbox mb{*this, "Msgbox"};
    mb.icon(mb.icon_information) << "Button Clicked";
    b_pl.events().click(mb);
    b_s.events().click(mb);
    b_n.events().click(mb);
    b_pr.events().click(mb);
    b_rpl.events().click(mb);

    b_pl.tooltip("Play/Pause");
    b_s.tooltip("Stop");
    b_n.tooltip("Next");
    b_pr.tooltip("Previous");
    b_rpl.tooltip("Set the Replay");

    b_pl.enable_pushed(true);
    b_s.enable_pushed(true);
    b_n.enable_pushed(true);
    b_pr.enable_pushed(true);
    b_rpl.enable_pushed(true);

    b_pl.icon(paint::image("D:\HSE\Prog\foraudioplayer\1.bmp"));
    //element::bground bground1;
    //element::bground bground2;
    //element::bground bground3;
    //element::bground bground4;
    //element::bground bground5;

    //bground1.image(paint::image("../media/pl-ps.bmp"), true, {});
    //bground2.image(paint::image("../media/st.bmp"), true, {});
    //bground3.image(paint::image("../media/n.bmp"), true, {});
    //bground4.image(paint::image("../media/pr.bmp"), true, {});
    //bground4.image(paint::image("../media/rpl.bmp"), true, {});
  }
  /* function that enables all control buttons and places them in a resembling "+" order
                 * and creates a bckgr image for each button */
  void m_make_menus()
  {
    mnbr.push_back("&ADD");
    mnbr.at(0).append("Add A File", [this](menu::item_proxy &ip) {
      auto fs = m_pick_file(true);
      if (!fs.empty())
        lbx.at(0).append(fs); //надо чтобы он выводил на лбх не сам файл, а его имя...
    });
    mnbr.push_back("I&NFO");
    mnbr.at(1).append("About Us", [this](menu::item_proxy &) {
      msgbox mb{*this, "Msgbox"};
      mb.icon(mb.icon_information) << "Something About Us";
    });
  }
  /* function that creates 2 caregories in menu bar - adding a file and getting the information about the app
   *   the format of added file is path of the file*/
  void m_init_submain()
  {
    submn.div("margin=5 <bvmin margin=[5,15]> <slider weight=40% margin=[10,5]> <bvmax margin=[5,15]> <beq margin=[5,15]>"); //vert bvmin progress bvmax beq gap=10 margin=5
    submn["bvmin"] << b_vmin;
    submn["slider"] << sldr;
    submn["bvmax"] << b_vmax;
    submn["beq"] << b_eq;
    msgbox mb{*this, "Msgbox"};
    mb.icon(mb.icon_information) << "BUTTON CLICKED!";
    b_vmin.events().click(mb);
    b_vmax.events().click(mb);
    b_eq.events().click(mb);

    b_vmin.tooltip("Minimize the Volume");
    b_vmax.tooltip("Maximize the Volume");
    b_eq.tooltip("Equalizer");

    b_vmin.enable_pushed(true);
    b_vmax.enable_pushed(true);
    b_eq.enable_pushed(true);

    //element::bground bground1;
    //element::bground bground2;
    //element::bground bground3;

    //bground1.image(paint::image("../media/vmin.bmp"), true, {});
    //bground2.image(paint::image("../media/vmax.bmp"), true, {});
    //bground3.image(paint::image("../media/eq.bmp"), true, {});

    tmr.elapse([this](const nana::arg_elapse &a) {
      if (sldr.value() == sldr.maximum())
      {
        sldr.value(0);
        tmr.reset();
      }
      sldr.move_step(true);
      tmr.start();
      //  prg.inc();
    });
    //prg.events().click((int x, int y){
    //  prg.
    //});
    //tmr.interval(std::chrono::milliseconds{80});
    //tmr.start();
    //if (lbx.events().selected()) tmr.start();
  }
  /* function that initializes the buttons and slider in the field submn
   * a timer is set to determine the length of the track and show its progress
   * the problem for now is unability to connect the click.event on listos to give a signal too 
   * when it wants to go there*/
  //void m_init_listbox()
  //{
  //I didn't find the right place for this method and how to coonect it to the listbox
  //}
  /* not finished func, didn't fifnish because all above stuff doesn't work - i cannot launch it
          *the main purpose - to provide the path of track to a variable so that it could be played
          *  right from the directory*/
};

int main()
{

  fm wdw1;

  wdw1.show();
  //paint::image img("../media/111.bmp");
  //drawing dw(wdw1);
  //    dw.draw([&img](nana::paint::graphics & graph)
  //    {
  //        img.paste(graph, 0, 0);
  //    });
  //    dw.update();
  exec();
}

/* void m_pickfile_test() {
  TEST_CASE("sOMETHING WORKS")
  REQUIRE(m_pick_file(true) == std::filesystem::path{});
  REQUIRE(m_pick_file(false) == "")
}
}*/