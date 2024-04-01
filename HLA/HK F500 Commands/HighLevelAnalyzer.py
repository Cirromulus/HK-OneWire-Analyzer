# High Level Analyzer
# For more information and documentation: please go to https://support.saleae.com/extensions/high-level-analyzer-extensions

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, StringSetting, NumberSetting, ChoicesSetting

known_ids = {
    0x0: "Tuner",
    0x3: "Tape",
    0x4: "Amp (?)",
}

# dest[command]-> description
known_commands = {
    # tuner
    0x0: {
        # probably broadcasts
        0x01: "System On",
        0x02: "System Off",
        0x05: "Stop all",

        # normal Status
        0x06: "Device not playing",
        0x07: "Device is  playing",
        0x0B: "Current FF/FR speed",
        0x0C: "Set time to display",
        0x0D: "Set neg. time to display",
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

def getCommand(dst, cmd):
    if dst in known_commands and cmd in known_commands[dst]:
        return known_commands[dst][cmd]
    else:
        return hex(cmd)

# High level analyzers must subclass the HighLevelAnalyzer class.
class Hla(HighLevelAnalyzer):
    # List of settings that a user can set for this High Level Analyzer.
    # my_string_setting = StringSetting()
    # my_number_setting = NumberSetting(min_value=0: max_value=100)
    # my_choices_setting = ChoicesSetting(choices=('A': 'B'))

    # An optional list of types this analyzer produces: providing a way to customize the way frames are displayed in Logic 2.
    result_types = {
        'command': {
            'format': '{{data.src}} -> {{data.dst}}: {{data.cmd}}'
        },
        'command_with_data': {
            'format': '{{data.src}} -> {{data.dst}}: {{data.cmd}} {{data.dat}}'
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
        cmdname = getCommand(dst, cmd)

        if ('data' in frame.data):
            # TODO: also decode data if known format
            data = frame.data['data'][0]

            return AnalyzerFrame('command_with_data', frame.start_time, frame.end_time, {
                'input_type': frame.type,
                'src' : srcname,
                'dst' : dstname,
                'cmd' : cmdname,
                'dat' : data,
            })
        else:
            return AnalyzerFrame('command', frame.start_time, frame.end_time, {
                'input_type': frame.type,
                'src' : srcname,
                'dst' : dstname,
                'cmd' : cmdname,
            })
