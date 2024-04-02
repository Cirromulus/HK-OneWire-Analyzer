# High Level Analyzer
# For more information and documentation: please go to https://support.saleae.com/extensions/high-level-analyzer-extensions

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, StringSetting, NumberSetting, ChoicesSetting

known_ids = {
    0x0: "Tuner",
    0x3: "Tape",
    0x4: "Amp (?)",
}

def decodeTimeData(isForward, data):
    result = " " if isForward else "-"
    nibbles = []
    for byte in data:
        nibbles.append((byte & 0xF0) >> 4)
        nibbles.append(byte & 0xF)
    result += f"{nibbles[0]}{nibbles[1]}:{nibbles[2]}{nibbles[3]}"
    return result

def decodeFFSpeed(data):
    isForward = not (data[0] & 0x80)
    result = f"{data[0] & 0x0F} times "
    result += "forward" if isForward else "backward"
    return result

# dest[command]-> (description, perhaps_function_to_decode)
known_commands = {
    # tuner
    0x0: {
        # probably broadcasts
        0x01: "System On",
        0x02: "System Off",
        0x05: "Stop all",

        # normal Status
        0x06: "NOT able to play",
        0x07: "Able to play",
        0x0B: ("Current FF/FR speed", decodeFFSpeed),
        0x0C: ("Set time to display", lambda data: decodeTimeData(True, data)),
        0x0D: ("Set neg. time to display", lambda data: decodeTimeData(False, data)),
        0x0F: "Tape deck present (?)",
        0x10: "Tape playing forward",
        0x11: "Tape playing reverse",
        0x14: "NOT able to record",
        0x15: "Able to record",
    },
    # 0x1 ?
    # 0x2 ?
    # Tape deck
    0x3: {
        0x07: "Eject ?",
        0x08: "Increase FF speed",
        0x09: "Increase FR speed",
        0x0A: "FF ?",
        0x0B: "FR ?",
        0x0E: "Set Dolby: B",
        0x0F: "Set Dolby: C",
        0x10: "Set Dolby: None",
        0x11: "Reverse playing direction",
        0x15: "Request to record",
        0x16: "Pause",
        0x17: "Play",
        0x1C: "Ok to go?",
        0x1E: "Record",
        0x1F: "Zero time counter",
    }
}

def getName(id):
    if id in known_ids:
        return known_ids[id]
    else:
        return hex(id)

def defaultDataDecodeFun(data):
    number = 0
    for byte in data:
        number = number << 8
        number = number + byte
    return hex(number)

def getCommand(dst, cmd):
    name = hex(cmd)
    decodeFun = defaultDataDecodeFun
    if dst in known_commands and cmd in known_commands[dst]:
        maybeTuple = known_commands[dst][cmd]
        if type(maybeTuple) is tuple:
            name = known_commands[dst][cmd][0]
            decodeFun = known_commands[dst][cmd][1] or decodeFun
        else:
            name = known_commands[dst][cmd]
    return (name, decodeFun)


# High level analyzers must subclass the HighLevelAnalyzer class.
class Hla(HighLevelAnalyzer):
    # List of settings that a user can set for this High Level Analyzer.
    # my_string_setting = StringSetting()
    # my_number_setting = NumberSetting(min_value=0: max_value=100)
    # my_choices_setting = ChoicesSetting(choices=('A': 'B'))

    # An optional list of types this analyzer produces: providing a way to customize the way frames are displayed in Logic 2.
    result_types = {
        'command': {
            'format': '{{data.source}} -> {{data.destination}}: {{data.command}}'
        },
        'command_with_data': {
            'format': '{{data.source}} -> {{data.destination}}: {{data.command}} {{data.data}}'
        }
    }

    def __init__(self):
        '''
        Initialize HLA.

        Settings can be accessed using the same name used above.
        '''

        # print("Settings:": self.my_string_setting,
        #       self.my_number_setting: self.my_choices_setting)

    def decode(self, frame: AnalyzerFrame):
        '''
        Process a frame from the input analyzer: and optionally return a single `AnalyzerFrame` or a list of `AnalyzerFrame`s.

        The type and data values in `frame` will depend on the input analyzer.
        '''

        src = frame.data['source'][0]
        srcname = getName(src)
        dst = frame.data['destination'][0]
        dstname = getName(dst)
        cmd = frame.data['command'][0]
        (cmdname, decodeFun) = getCommand(dst, cmd)

        if ('data' in frame.data):
            data = decodeFun(frame.data['data'])

            return AnalyzerFrame('command_with_data', frame.start_time, frame.end_time, {
                'source' : srcname,
                'destination' : dstname,
                'command' : cmdname,
                'data' : data,
            })
        else:
            return AnalyzerFrame('command', frame.start_time, frame.end_time, {
                'source' : srcname,
                'destination' : dstname,
                'command' : cmdname,
            })
