#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND   "Epsilon Audio"
#define DISTRHO_PLUGIN_NAME    "Monolith"
#define DISTRHO_PLUGIN_URI     "https://epsilonrecords.com/plugins/monolith"
#define DISTRHO_PLUGIN_CLAP_ID "com.epsilonrecords.monolith"

#define DISTRHO_PLUGIN_BRAND_ID  EpsA
#define DISTRHO_PLUGIN_UNIQUE_ID Mnl1

#define DISTRHO_PLUGIN_HAS_UI           1
#define DISTRHO_UI_USE_NANOVG           1
#define DISTRHO_UI_USER_RESIZABLE       0
#define DISTRHO_UI_DEFAULT_WIDTH        980
#define DISTRHO_UI_DEFAULT_HEIGHT       520
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_IS_SYNTH         1
#define DISTRHO_PLUGIN_NUM_INPUTS       0
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_PROGRAMS    1

#define DISTRHO_PLUGIN_VST3_CATEGORIES "Instrument|Synth|Mono"
#define DISTRHO_PLUGIN_CLAP_FEATURES   "instrument", "synthesizer", "mono"
#define DISTRHO_PLUGIN_LV2_CATEGORY    "lv2:InstrumentPlugin"

#endif
