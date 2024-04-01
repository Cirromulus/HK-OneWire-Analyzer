#include "HKWire.h"

using namespace HKWire;

const IdDescriptionMap HKWire::knownIDs =
{
    {0x0, "Tuner"},
    {0x3, "Tape"},
    {0x4, "Amp (?)"},
};

const CommandDescriptionMap ampCommands =
{
    {0x06, "set off?"},
    {0x07, "set mute??"},
    {0x13, "set input to Tape?"},
};

const CommandDescriptionMap tunerCommands =
{
    {0x01, "System On"},
    {0x02, "System Off"},
    {0x10, "Tape playing forward"},
    {0x11, "Tape playing reverse"},
    {0x0B, "Current FF/FR speed"},
    {0x0C, "Set time to display"},
    {0x0D, "Set neg. time to display"},
    {0x06, "Device not playing"},
    {0x07, "Device is  playing"},
};

const CommandDescriptionMap tapeDeckCommands =
{
    {0x07, "Eject ?"},
    {0x08, "Increase FF speed"},
    {0x09, "Increase FR speed"},
    {0x0A, "FF ?"},
    {0x0B, "FR ?"},
    {0x0E, "Set Dolby: B"},
    {0x0F, "Set Dolby: C"},
    {0x10, "Set Dolby: None"},
    {0x11, "Reverse playing direction"},
    {0x15, "\"record awareness\"?"},
    {0x1E, "Record"},
    {0x16, "Pause"},
    {0x17, "Play"},
    {0x1C, "Ok to go?"},
    {0x1F, "Zero time counter"},
};

const TargetCommandDescriptionMap HKWire::knownCommands =
{
    {0x0, tunerCommands},
    {0x3, tapeDeckCommands},
    {0x4, ampCommands},
};