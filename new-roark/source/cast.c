// cast.c - everyone who can talk. id = portrait file name (gfx/<id>.png), name = what shows in the name tag.
#include "story.h"
#include <string.h>

const CastMember CAST[] = {
    // Wolf
    {"wendy","Wendy",0xE8603C}, {"whitney","Whitney",0x3B4E8C}, {"willow","Willow",0xC89B5A},
    {"waverly","Waverly",0xD9433A}, {"winter","Winter",0xE58FB4}, {"watts","Watts",0x8E9AA6},
    {"vanya","Mom (Vanya)",0x7E7A93}, {"wyatt","Dad (Wyatt)",0x6B7A3C},
    // Dobermann
    {"danny","Danny",0x5A5A5A}, {"derek","Mr. Dobermann",0x2B3550}, {"diana","Diana",0x8FA8D8},
    // Pony
    {"pedro","Pedro",0x5E7A52}, {"patricia","Patricia",0xD9452F}, {"paloma","Paloma",0x7A3E6A},
    {"mrpony","Mr. Pony",0x7FA3C9}, {"mrspony","Mrs. Pony",0xA0582A},
    // Mole
    {"molly","Molly",0xB0262A}, {"malachi","Malachi",0xB58D4A}, {"mrmole","Mr. Mole",0x3C4E7A}, {"mrsmole","Mrs. Mole",0xB5794B},
    // Sheep
    {"suzy","Suzy",0xC9A0A8}, {"mrssheep","Mrs. Sheep",0xB98A9C}, {"slater","Slater",0x34405A},
    // Pig
    {"peppa","Peppa",0xE0559B}, {"george","George",0x3E5A3A}, {"mrspig","Mrs. Pig",0x6FA0D8}, {"mrpig","Mr. Pig",0xA88A62},
    // Kameelperd (giraffe)
    {"gerald","Gerald",0xD99A1E}, {"keva","Keva",0xC9B48A}, {"kaya","Kaya",0x7FA7D8}, {"kanyisa","Kanyisa",0x9A9A9A},
    {"mrk","Mr. K",0x5A4E46}, {"mrsk","Mrs. K",0xBFA26A},
    // Fox
    {"freddy","Freddy",0xE07818}, {"fraya","Fraya",0xE58FA8}, {"mrfox","Mr. Fox",0x4A3326}, {"mrsfox","Mrs. Fox",0xE07AA0},
    // Rabbit
    {"rebecca","Rebecca",0x7FA2D6}, {"richard","Richard",0x444444}, {"rosie","Rosie",0x8A5A4A}, {"robbie","Robbie",0x2F5E3A},
    {"mrrabbit","Mr. Rabbit",0x5A86D6}, {"mrsrabbit","Mrs. Rabbit",0xB0263A},
    // Cat
    {"candy","Candy",0xE88A2A}, {"mscat","Ms. Cat",0xC9A21E},
    // Kangaroo
    {"karter","Karter",0x2C3E66}, {"katrina","Mrs. Kangaroo",0x8A6A8A},
    // Elephant
    {"emily","Emily",0x8A62B8}, {"mrelephant","Mr. Elephant",0x2E4A3C}, {"mrselephant","Mrs. Elephant",0xC0662E},
    // Squirrel
    {"simon","Simon",0x5A2E8A}, {"samantha","Samantha",0x3A3F55}, {"mrsquirrel","Mr. Squirrel",0x2E6A4A}, {"mrssquirrel","Mrs. Squirrel",0xB8302E},
    // The Baddies sheet + extras
    {"kylie","Kylie",0xE0703A}, {"peggi","Peggi",0x8FB4E0}, {"evie","Evie",0xF29AC8},
    {"rebecca_b","Rebecca",0x7FA2D6}, {"peppa_home","Peppa",0xE0559B},
    // voices with no portrait
    {"narr","",0x000000}, {"phone","Phone",0x4A90D9}, {"teacher","Teacher",0x6A6A6A},
    {"radio","Radio",0x6A6A6A}, {"tv","TV",0x6A6A6A}, {"crowd","Crowd",0x6A6A6A}, {"kid","Kid",0x6A6A6A},
};
const int CAST_COUNT = sizeof CAST / sizeof CAST[0];

const CastMember *cast_find(const char *id) {
    for (int i = 0; i < CAST_COUNT; i++)
        if (strcmp(CAST[i].id, id) == 0) return &CAST[i];
    return NULL;
}
