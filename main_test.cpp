
#define CATCH_CONFIG_RUNNER

#include "catch.hpp"
#include "fmod_functions.hpp"
#include <fmod.hpp>
#include "common.h"
#include <stdexcept>

FMOD::System *system2;
FMOD::Sound *sound;
FMOD::Channel *channel = 0;

TEST_CASE("play existing file") {
	REQUIRE(play_sound_(system2, sound, channel, ".\\media\\meow.mp3") == FMOD_OK);
}

TEST_CASE("play not existing file") {
	FMOD_RESULT res = play_sound_(system2, sound, channel, ".\\media\\meooooow.mp3");
	REQUIRE(res != FMOD_OK);
}

TEST_CASE("go to begin of the track") {
	FMOD_RESULT res = begin_of_the_track_(channel);
	unsigned t;
	channel->getPosition(&t, FMOD_TIMEUNIT_MS);
	bool b = (0 <= t && t < 10);
	REQUIRE(b);
}


int FMOD_Main(int argc, char **argv) {
	void *extradriverdata = 0;
	Common_Init(&extradriverdata);

	FMOD::System_Create(&system2);
	system2->init(32, FMOD_INIT_NORMAL, extradriverdata);
	system2->createSound(Common_MediaPath("meow.mp3"), FMOD_CREATESTREAM, 0, &sound);
	sound->setMode(FMOD_LOOP_OFF);
	int result = Catch::Session().run(argc, argv);

	sound->release();
	system2->close();
	system2->release();

	Common_Close();
	return result;
}
