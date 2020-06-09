//
// Created by lukas on 08.06.2020.
//

#ifndef SOUND_FUNCTIONS_FOR_TEST_HPP
#define SOUND_FUNCTIONS_FOR_TEST_HPP

#include <fmod.hpp>
#include <exception>
#include <algorithm>
#include "common.h"
//#include "common_platform.h"

/**
 * Проверка на корректность результата
 * @param res - FMOD_RES
 * Кидает исключение, если FMOD_RESULT != FMOD_OK
 */
void ERROR_CHECK(FMOD_RESULT const &res) {
	if (res != FMOD_OK) {

		//throw std::runtime_error("Fatal error, 99% that file is not found.");
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
FMOD_RESULT _play_sound_(FMOD::System *&system, FMOD::Sound *&sound, FMOD::Channel *&channel, char *path) {
	int q = 0;
	FMOD_RESULT result;
	result = system->getChannelsPlaying(&q, nullptr);
	ERROR_CHECK(result);
	if (q > 0) {
		channel->stop();
	}
	result = system->createSound(Common_MediaPath(path), FMOD_CREATESTREAM, 0, &sound);
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
FMOD_RESULT _pause_the_sound_(FMOD::Channel *&channel) {
	FMOD_RESULT result;
	bool paused;
	result = channel->getPaused(&paused);
	ERROR_CHECK(result);
	result = channel->setPaused(!paused);
	return result;
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

#endif //SOUND_FUNCTIONS_FOR_TEST_HPP
