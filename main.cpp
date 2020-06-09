/**
 * \file
 * \author Lukashov Sergey, Belousov Dmitry, Kosmachev Alexey
 */
#include "fmod.hpp"
#include "common.h"
#include <exception>
#include <algorithm>
#include <fmod_dsp_effects.h>
#include <string>

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
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/spinbox.hpp>
#include <nana/gui/widgets/textbox.hpp>


FMOD::System *system1;
FMOD::ChannelControl *chanel_control1 = 0;
FMOD::Sound *sound1;
FMOD::ChannelGroup *mastergroup = 0;
FMOD::Channel *channel1 = 0;
FMOD::DSP *highpass_dsp = 0;
FMOD::DSP *lowpass_dsp = 0;
FMOD::DSP *echo_dsp = 0;
FMOD::DSP *flange_dsp = 0;


/**
 * Проверка на корректность результата
 * @param res - FMOD_RES
 * Кидает исключение, если FMOD_RESULT != FMOD_OK
 */
void ERROR_CHECK(FMOD_RESULT const &res) {
	if (res != FMOD_OK) {
		throw std::runtime_error("Fatal error, 99% that file is not found.");
	}
}

/**
 * \brief функция для воспроизведения звука в выбранном канале
 * @param system - указатель на систему
 * @param sound - указатель на созданный звук
 * @param channel - указатель на канал, в котором будет проигрываться звук
 * @param path - путь, по которому искать трек
 * @return FMOD_RESULT
 */
FMOD_RESULT _play_sound_(FMOD::System *&system, FMOD::Sound *&sound, FMOD::Channel *&channel, char const *path) {
	int q = 0;
	FMOD_RESULT result;
	result = system->getChannelsPlaying(&q, nullptr);
	ERROR_CHECK(result);
	if (q > 0) {
		channel->stop();
	}
	result = system->createSound(path, FMOD_CREATESTREAM, 0, &sound);
	ERROR_CHECK(result);
	result = (sound)->setMode(FMOD_LOOP_OFF);
	ERROR_CHECK(result);
	result = system->playSound(sound, 0, false, &channel);
	return result;
}

/**
 * \brief Перемотка вперед
 * @param len_ms на сколько надо перемотать вперед (мс)
 * @return FMOD_RESULT
 */
FMOD_RESULT _increase_time_(FMOD::Sound *&sound, FMOD::Channel *&channel, unsigned int len_ms) {
	FMOD_RESULT result;
	unsigned int len, max_len;
	result = channel->getPosition(&len, FMOD_TIMEUNIT_MS);
	ERROR_CHECK(result);
	result = sound->getLength(&max_len, FMOD_TIMEUNIT_MS);
	ERROR_CHECK(result);
	result = channel->setPosition(std::min<unsigned>(len + len_ms, max_len), FMOD_TIMEUNIT_MS);
	return result;
}

/**
 * \brief Перемотка назад
 * @param len_ms на сколько надо перемотать назад (мс)
 * @return FMOD_RESULT
 */
FMOD_RESULT _decrease_time_(FMOD::Sound *&sound, FMOD::Channel *&channel, unsigned int len_ms) {
	FMOD_RESULT result;
	unsigned int len, max_len;
	result = channel->getPosition(&len, FMOD_TIMEUNIT_MS);
	ERROR_CHECK(result);
	result = sound->getLength(&max_len, FMOD_TIMEUNIT_MS);
	ERROR_CHECK(result);
	if (len < len_ms) {
		result = channel->setPosition(0, FMOD_TIMEUNIT_MS);
	} else {
		result = channel->setPosition(len - len_ms, FMOD_TIMEUNIT_MS);
	}
	return result;
}


/**
 * \brief останавливает выбранный канал
 * @param channel
 * @return FMOD_RESULT
 */
void _pause_the_sound_(FMOD::Channel *&channel) {
	FMOD_RESULT result;
	bool paused;
	result = channel->getPaused(&paused);
	ERROR_CHECK(result);
	result = channel->setPaused(!paused);
	//return result;
}


void _stop_the_sound_(FMOD::Channel *&channel) {
	FMOD_RESULT result;
	result = channel->setPaused(true);
	//return result;
}

/**
 *перематывает трек в начало
 * @return FMOD_RESULT
 */
FMOD_RESULT _begin_of_the_track_(FMOD::Channel *&channel) {
	FMOD_RESULT result;
	result = channel->setPosition(0, FMOD_TIMEUNIT_MS);
	return result;
}


/**
 * \brief позволяет очутиться в определённом месте трека(выражается в проуентах от начала)
 * @param system
 * @param sound
 * @param channel
 * @param percent
 * @return FMOD_RESULT
 */
FMOD_RESULT _move_in_track_(FMOD::System *&system, FMOD::Sound *&sound, FMOD::Channel *&channel, float const &percent) {
	FMOD_RESULT result;
	result = _begin_of_the_track_(channel);
	ERROR_CHECK(result);
	unsigned int full_time = 0;
	result = sound->getLength(&full_time, FMOD_TIMEUNIT_MS);
	ERROR_CHECK(result);
	float new_time = percent * full_time;
	result = _increase_time_(sound, channel, new_time);
	return result;
}

/**
 * увеличивает громкость на 5%
 * @param channel
 * @return FMOD_RESULT
 */
FMOD_RESULT _increse_volume_(FMOD::Channel *&channel) {
	FMOD_RESULT result;
	float vol = 0;
	result = channel->getVolume(&vol);
	ERROR_CHECK(result);
	result = channel->setVolume(std::min<unsigned>(vol + 0.05, 1));
	return result;
}

/**
 * уменьшает громкость на 5%
 * @param channel
 * @return FMOD_RESULT
 */
FMOD_RESULT _decrease_volume_(FMOD::Channel *&channel) {
	FMOD_RESULT result;
	float vol = 0;
	result = channel->getVolume(&vol);
	ERROR_CHECK(result);
	result = channel->setVolume(std::max<unsigned>(vol - 0.1, 0));
	return result;
}

/**
 * \brief заглушает звук в канале
 * @param channel
 * @return FMOD_RESULT
 */
FMOD_RESULT _mute_(FMOD::Channel *&channel) {
	FMOD_RESULT result;
	result = channel->setVolume(0);
	return result;
}

/**
 * \brief позволяет изменить громкость на выбранную величину в %)
 * @param channel
 * @param dif
 * @return FMOD_RESULT
 */
FMOD_RESULT _change_volume_(FMOD::Channel *&channel, float dif) {
	FMOD_RESULT result;
	float vol = 0;
	result = channel->getVolume(&vol);
	ERROR_CHECK(result);
	result = channel->setVolume(std::min<float>(1.0, std::max<float>(vol + dif, 0)));
	return result;
}

/**
 * changes the dsp parametr
 * @param dsp - dsp
 * @param freq - максимальная частота, которая будет проигрываться(для dsp = FMOD_DSP_TYPE_LOWPASS), минимальная частота, которая будет проигрываться(для dsp = FMOD_DSP_TYPE_HIGHPASS)
 * @return FMOD_RESULT
 */
FMOD_RESULT _FMOD_change_lowpass_or_highpass_parameter_(FMOD::DSP *&dsp, float const &freq = 0) {
	FMOD_RESULT result;
	result = dsp->setParameterFloat(0, freq);
	return result;
}

/**
 * Изменяет активность dsp
 * @param dsp - выбранный dsp
 * @return FMOD_RESULT
 */
FMOD_RESULT _change_dsp_bypass_(FMOD::DSP *&dsp) {
	FMOD_RESULT result;
	bool bypass;
	result = dsp->getBypass(&bypass);
	ERROR_CHECK(result);
	result = dsp->setBypass(!bypass);
	return result;
}

using namespace nana;


/**
	void equalizer() - function that shows the equalizer's menu with all icluded settings
	----
	makes a window
	allows user to on/off echo/flange (fixing buttons)
	allows user to cut low and high frequences (of input values of cut frequences)
	allows user to choose reverb with a slider
	allows user to make some notes
 * \author Kosmachev Alexey
*/
void equalizer() {


	/*
	----Creating window----
	*/


	// size of the window
	const rectangle eq_rect = API::make_center(600, 250);

	// creating the window of determined size
	form equa(eq_rect);
	equa.caption("Equalizer"); //giving name to the window


	/*
	----Creating note field----
	*/


	/// field for notes
	textbox notes{equa};
	notes.editable(true); // allow user to edit notes

	//message to make user understand where the notes are
	label note_label{equa, "Notes:"};
	note_label.text_align(align::right, align_v::center);


	/*
	----Creating Echo/Flange buttons----
	*/


	///message to make user understand where the echo enable/disable button is
	label echo_label{equa, "Echo:"};
	echo_label.text_align(align::right, align_v::center);

	///message to make user understand where the flange enable/disable button is
	label flange_label{equa, "Flange:"};
	flange_label.text_align(align::right, align_v::center);

	///creating echo button

	//----------------------------------------------------------------------------------------------------------
	button echo_btn{equa};
	bool bypass2 = true;
	echo_dsp->getBypass(&bypass2);
	echo_btn.caption(bypass2 ? "Off" : "On");
	echo_btn.enable_pushed(false);

	///taking actions when the button is clicked (changing its status + enable action)
	echo_btn.events().click([&] {
		echo_btn.enabled(false); //disable button while taking actions
		if (echo_btn.pushed()) { //if already pushed..
			_change_dsp_bypass_(echo_dsp);
			bool bypass = true;
			echo_dsp->getBypass(&bypass);
			echo_btn.caption(bypass ? "Off" : "On");
			echo_btn.enable_pushed(false);
		} else {
			_change_dsp_bypass_(echo_dsp);
			bool bypass = true;
			echo_dsp->getBypass(&bypass);
			echo_btn.caption(bypass ? "Off" : "On");
			echo_btn.enable_pushed(true);
			/*
				... /some code/ ...
			*/
		}
		echo_btn.enabled(true); //enable button again
	});

	//creating flange button
	button flange_btn{equa};

	bool bypass1 = true;
	flange_dsp->getBypass(&bypass1);
	flange_btn.caption(bypass1 ? "Off" : "On");
	flange_btn.enable_pushed(false);

	//taking actions when the button is clicked (changing its status + enable action)
	flange_btn.events().click([&] {
		flange_btn.enabled(false); //disable button while taking actions
		if (flange_btn.pushed()) { //if already pushed..
			_change_dsp_bypass_(flange_dsp);
			bool bypass = true;
			flange_dsp->getBypass(&bypass);
			flange_btn.caption(bypass ? "Off" : "On");
			flange_btn.enable_pushed(false);

		} else {
			_change_dsp_bypass_(flange_dsp);
			bool bypass = true;
			flange_dsp->getBypass(&bypass);
			flange_btn.caption(bypass ? "Off" : "On");
			flange_btn.enable_pushed(true);
		}
		flange_btn.enabled(true); //enable button again
	});


	/*
	----Creating frequency cut fields----
	*/

	//--low frquency

	//message to make user understand where to cut frequences
	label frequency_announce_label{equa, "Frquency cutting:"};
	frequency_announce_label.text_align(align::center, align_v::center);

	//message to tell user what physical quantities to use with sound frequency (Hz)
	label high_herz_label{equa, "Hz"};
	high_herz_label.text_align(align::right, align_v::center);

	//message to tell user what physical quantities to use with sound frequency (Hz)
	label low_herz_label{equa, "Hz"};
	low_herz_label.text_align(align::right, align_v::center);

	//message to tell user where to set low frequency cut
	label low_frequency_label{equa, "Low frquency cut:"};
	low_frequency_label.text_align(align::left, align_v::center);

	//creating a field for inputing low frequency cut
	spinbox low_frequencies_spin{equa};
	low_frequencies_spin.range(20, 20000, 1); //setting range
	low_frequencies_spin.editable(true); //making it possible to insert input value

	//creating a button to set low frequency cut
	button low_freq_button{equa};
	low_freq_button.caption("Cut!");

	//taking actions when the button is clicked
	low_freq_button.events().click([&] {
		low_freq_button.enabled(false); //disable button while taking actions
		float low_cut = low_frequencies_spin.to_int(); //accepting the given value

		_FMOD_change_lowpass_or_highpass_parameter_(highpass_dsp, low_cut);
		_change_dsp_bypass_(highpass_dsp);

		low_freq_button.enabled(true); //enable button again
	});

	//--high frequency

	//message to tell user where to set high frequency cut
	label high_frequency_label{equa, "High frquency cut:"};
	high_frequency_label.text_align(align::left, align_v::center);

	//creating a field for inputing high frequency cut
	spinbox high_frequencies_spin{equa};
	high_frequencies_spin.range(20, 20000, 1);
	high_frequencies_spin.editable(true);

	//creating a button to set high frequency cut
	button high_freq_button{equa};
	high_freq_button.caption("Cut!");

	//taking actions when the button is clicked
	high_freq_button.events().click([&] {
		high_freq_button.enabled(false); //disable button while taking actions
		float high_cut = high_frequencies_spin.to_int(); //accepting the given value
		_FMOD_change_lowpass_or_highpass_parameter_(lowpass_dsp, high_cut);
		_change_dsp_bypass_(lowpass_dsp);

		high_freq_button.enabled(true); //enable button again
	});


	/*
	----Creating reverb field----ен
	*/


	//message to tell user where to set reverb value
	label reverb_label{equa, "Convolution reverb:"};
	reverb_label.text_align(align::center, align_v::center);

	//creating a slider to allow user to chose reverb value
	slider sld{equa};

	//creating a button for setting reverb value
	button reverb_button{equa};
	reverb_button.caption("Reverb!");

	//taking actions when the button is clicked
	reverb_button.events().click([&] {
		reverb_button.enabled(false); //disable button while taking actions
		sld.maximum(10000); //setting maximum (we will get % in float with two symbols after dot)
		float reverb_value = sld.value();  //accepting the given value
		reverb_value /= 100; //counting value
		/*
			.../some code/...
		*/
		reverb_button.enabled(true); //enable button again
	});


	/*
	----Showing the elements----
	*/


	//placing all the elements (reserve place)
	place plc{equa};
	plc.div("vert <weight=35 margin=5 <arrange=[40,40] gap=10 echo><arrange=[40,40] gap=10 flange>>" // echo/flange buttons

			"<weight=25 margin=[5, 20] arrange=[variable] freq_announce>" // frequency cut announce

			"<weight=35 margin=[5, 20] arrange=[100, 40, variable, 40] gap = [10, 20, 10] high_frquency_cut>" // high frequency cut

			"<weight=35 margin=[5, 20] arrange=[100, 40, variable, 40] gap = [10, 20, 10] low_frquency_cut>" // low frequency cut

			"<weight=25 margin=[5, 20] arrange=[variable] reverb_label>" // reverb announce

			"<weight=35 margin=[5, 20] arrange=[45, variable] gap = 20 reverb>" //reverb

			"<weight=45 margin=5 arrange=[40,variable] gap=7 notes> " //notes
	);

	//placing the objects themselves

	// echo button
	plc["echo"] << echo_label << echo_btn;

	//flange button
	plc["flange"] << flange_label << flange_btn;

	//frequency cut announce
	plc["freq_announce"] << frequency_announce_label;

	//high frequency cut
	plc["high_frquency_cut"] << high_frequency_label << high_freq_button << high_frequencies_spin << high_herz_label;

	//low frequency cut
	plc["low_frquency_cut"] << low_frequency_label << low_freq_button << low_frequencies_spin << low_herz_label;

	//reverb announce
	plc["reverb_label"] << reverb_label;

	//reverb button
	plc["reverb"] << reverb_button << sld;

	//notes
	plc["notes"] << note_label << notes;

	//building all
	plc.collocate();

	//showing the window
	equa.show();

	//exit when the window is closed
	nana::exec();
}


/** класс для более удобного оформления элементов окна, которые собраны в группы(такие как кнопки и кнопки с
				  * прогресс-баром), либо описаны самостоятельными
				  * в public описываются параметры поля окна, его подпространства, в private - функции без
					возвращаемых значений, осуществляющих работу с элементами класса
				-----------------------------------------------------------------------------------------------------------------
				\author Belousov Dmitry
					*/
class fm : public form {
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

public:
	fm()
			: form(API::make_center(400, 600),
				   appear::decorate<appear::minimize, appear::maximize, appear::sizable>()) //300,600
	{
		nana::API::track_window_size(*this, {400, 600}, false);
		nana::API::track_window_size(*this, {400, 600}, true);
		plc.div("vert <menubar weight=28><main weight=30%><listbox>");
		plc["menubar"] << mnbr;
		plc["main"] << mn;
		mn.div("<vert all min=260 gap=10 margin=10>"); //weight=50% gap=5 margin=10><weight=30%
		mn["all"] << bttns << submn;
		plc.field("listbox") << lbx;

		lbx.append_header("Songs' Headers", 400);
		lbx.events().selected(
				[&](const arg_listbox &arg) { /////////////////////////////////////////////////////////////////
					static auto ptrr = arg;
					auto t = arg.item.text(0);
					_play_sound_(system1, sound1, channel1, t.c_str());
				});

		m_init_buttons();
		// m_init_listbox();
		m_make_menus();
		m_init_submain();

		this->events().unload(
				[this](const arg_unload &ei) { // yes/no messagebox that opens when you try to exit the programme
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
	/**function that helps find the files with mp3 extension in the directory
	 *  of computer and return its path  */
	std::filesystem::path m_pick_file(bool is_open) const {
		filebox fbox(*this, is_open);
		fbox.add_filter("MP3", "*.mp3");
		fbox.add_filter("All Files", "*.*");
		auto files = fbox.show();
		return (files.empty() ? std::filesystem::path{} : files.front());
	}


	/** function that enables all control buttons and places them in a resembling "+" order
				   * and creates a bckgr image for each button */
	void m_init_buttons() {
		bttns.div("buttons gap=15 margin=[5,10]"); //grid=[3,3] collapse(2,2,3,2)
		bttns["buttons"] << b_s << b_pr << b_pl << b_n << b_rpl;
		msgbox mb{*this, "Msgbox"};
		mb.icon(mb.icon_information) << "Button Clicked";
		b_pl.events().click([&](const nana::arg_click &eventinfo) {
			_pause_the_sound_(channel1);
		});
		b_s.events().click([&](const nana::arg_click &eventinfo) {
			_stop_the_sound_(channel1);
		});
		//b_s.events().click(_stop_the_sound_(channel1));
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

		b_pl.icon(paint::image(".media\\111.bmp"));
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


	/** function that creates 2 categories in menu bar - adding a file and getting the information about the app
	 *   the format of added file is path of the file*/
	void m_make_menus() {
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

	void m_init_submain() {
		submn.div(
				"margin=5 <bvmin margin=[5,15]> <slider weight=40% margin=[10,5]> <bvmax margin=[5,15]> <beq margin=[5,15]>"); //vert bvmin progress bvmax beq gap=10 margin=5
		submn["bvmin"] << b_vmin;
		submn["slider"] << sldr;
		submn["bvmax"] << b_vmax;
		submn["beq"] << b_eq;
		msgbox mb{*this, "Msgbox"};
		mb.icon(mb.icon_information) << "BUTTON CLICKED!";
		b_vmin.events().click(mb);
		b_vmax.events().click(mb);
		b_eq.events().click([&]() { equalizer(); });

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
			if (sldr.value() == sldr.maximum()) {
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


int FMOD_Main(int argc, char **argv) {
	FMOD_RESULT result;
	unsigned int version;
	void *extradriverdata = 0;

	Common_Init(&extradriverdata);
	result = FMOD::System_Create(&system1);
	result = system1->getVersion(&version);
	if (version < FMOD_VERSION) {
		Common_Fatal("FMOD lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
	}
	result = system1->init(32, FMOD_INIT_NORMAL, extradriverdata);
	ERRCHECK(result);
	result = system1->getMasterChannelGroup(&mastergroup);

	result = system1->createSound(Common_MediaPath("meow.mp3"), FMOD_CREATESTREAM, 0, &sound1);
	result = sound1->setMode(FMOD_LOOP_OFF);

	ERRCHECK(result);
	system1->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &lowpass_dsp);
	system1->createDSPByType(FMOD_DSP_TYPE_HIGHPASS, &highpass_dsp);
	system1->createDSPByType(FMOD_DSP_TYPE_ECHO, &echo_dsp);
	system1->createDSPByType(FMOD_DSP_TYPE_FLANGE, &flange_dsp);

	mastergroup->addDSP(0, lowpass_dsp);
	mastergroup->addDSP(0, highpass_dsp);
	mastergroup->addDSP(0, echo_dsp);
	mastergroup->addDSP(0, flange_dsp);
	lowpass_dsp->setBypass(true);
	highpass_dsp->setBypass(true);
	echo_dsp->setBypass(true);
	flange_dsp->setBypass(true);
	try {
		fm wdw1;
		wdw1.show();
		exec();
	}
	catch (std::exception &e) {
		std::cout << "Something went wrong";
	}
	result = mastergroup->removeDSP(lowpass_dsp);
	ERRCHECK(result);
	result = mastergroup->removeDSP(highpass_dsp);
	ERRCHECK(result);
	result = mastergroup->removeDSP(echo_dsp);
	ERRCHECK(result);
	result = mastergroup->removeDSP(flange_dsp);
	ERRCHECK(result);

	result = lowpass_dsp->release();
	ERRCHECK(result);
	result = highpass_dsp->release();
	ERRCHECK(result);
	result = echo_dsp->release();
	ERRCHECK(result);
	result = flange_dsp->release();
	ERRCHECK(result);

	result = sound1->release(); //shut down
	result = system1->close();
	result = system1->release();
	Common_Close();


	return 0;
}
