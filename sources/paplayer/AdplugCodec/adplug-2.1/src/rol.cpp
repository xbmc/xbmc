/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * rol.h - ROL Player by OPLx <oplx@yahoo.com>
 *
 * Visit:  http://tenacity.hispeed.com/aomit/oplx/
 */
#include <algorithm>

#include "rol.h"
#include "debug.h"

int   const CrolPlayer::kSizeofDataRecord    =  30;
int   const CrolPlayer::kMaxTickBeat         =  60;
int   const CrolPlayer::kSilenceNote         = -12;
int   const CrolPlayer::kNumMelodicVoices    =  9;
int   const CrolPlayer::kNumPercussiveVoices = 11;
int   const CrolPlayer::kBassDrumChannel     =  6;
int   const CrolPlayer::kSnareDrumChannel    =  7;
int   const CrolPlayer::kTomtomChannel       =  8;
int   const CrolPlayer::kTomtomFreq          =  2;//4;
int   const CrolPlayer::kSnareDrumFreq       =  2;//kTomtomFreq + 7;
float const CrolPlayer::kDefaultUpdateTme    = 18.2f;
float const CrolPlayer::kPitchFactor         = 400.0f;

static const unsigned char drum_table[4] = {0x14, 0x12, 0x15, 0x11};

CrolPlayer::uint16 const CrolPlayer::kNoteTable[12] = 
{ 
    340, // C
    363, // C#
    385, // D
    408, // D#
    432, // E
    458, // F
    485, // F#
    514, // G
    544, // G#
    577, // A
    611, // A#
    647  // B
};

/*** public methods **************************************/

CPlayer *CrolPlayer::factory(Copl *newopl)
{
  return new CrolPlayer(newopl);
}
//---------------------------------------------------------
CrolPlayer::CrolPlayer(Copl *newopl)
:  CPlayer         ( newopl )
  ,rol_header      ( NULL )
  ,mNextTempoEvent ( 0 )
  ,mCurrTick       ( 0 )
  ,mTimeOfLastNote ( 0 )
  ,mRefresh        ( kDefaultUpdateTme )
  ,bdRegister      ( 0 )
{
    int n;

    memset(bxRegister,  0, sizeof(bxRegister) );
    memset(volumeCache, 0, sizeof(volumeCache) );
    memset(freqCache,   0, sizeof(freqCache) );

    for(n=0; n<11; n++)
      pitchCache[n]=1.0f;    
}
//---------------------------------------------------------
CrolPlayer::~CrolPlayer()
{
    if( rol_header != NULL )
    {
        delete rol_header;
        rol_header=NULL;
    }
}
//---------------------------------------------------------
bool CrolPlayer::load(const std::string &filename, const CFileProvider &fp)
{
    binistream *f = fp.open(filename); if(!f) return false;

    char *fn = new char[filename.length()+12];
    int i;
    std::string bnk_filename;

    AdPlug_LogWrite("*** CrolPlayer::load(f, \"%s\") ***\n", filename.c_str());
    strcpy(fn,filename.data());
    for (i=strlen(fn)-1; i>=0; i--)
      if (fn[i] == '/' || fn[i] == '\\')
	break;
    strcpy(fn+i+1,"standard.bnk");
    bnk_filename = fn;
    delete [] fn;
    AdPlug_LogWrite("bnk_filename = \"%s\"\n",bnk_filename.c_str());

    rol_header = new SRolHeader;
    memset( rol_header, 0, sizeof(SRolHeader) );

    rol_header->version_major = f->readInt( 2 );
    rol_header->version_minor = f->readInt( 2 );

    // Version check
    if(rol_header->version_major != 0 || rol_header->version_minor != 4) {
      AdPlug_LogWrite("Unsupported file version %d.%d or not a ROL file!\n",
	       rol_header->version_major, rol_header->version_minor);
      AdPlug_LogWrite("--- CrolPlayer::load ---\n");
      fp.close(f);
      return false;
    }

    f->seek( 40, binio::Add );

    rol_header->ticks_per_beat    = f->readInt( 2 );
    rol_header->beats_per_measure = f->readInt( 2 );
    rol_header->edit_scale_y      = f->readInt( 2 );
    rol_header->edit_scale_x      = f->readInt( 2 );

    f->seek( 1, binio::Add );

    rol_header->mode = f->readInt(1);

    f->seek( 90+38+15, binio::Add );

    rol_header->basic_tempo = f->readFloat( binio::Single );

    load_tempo_events( f );

    mTimeOfLastNote = 0;

    if( load_voice_data( f, bnk_filename, fp ) != true )
    {
      AdPlug_LogWrite("CrolPlayer::load_voice_data(f) failed!\n");
      AdPlug_LogWrite("--- CrolPlayer::load ---\n");

      fp.close( f );
      return false;
    }

    fp.close( f );

    rewind( 0 );
    AdPlug_LogWrite("--- CrolPlayer::load ---\n");
    return true;
}
//---------------------------------------------------------
bool CrolPlayer::update()
{
    if( mNextTempoEvent < mTempoEvents.size() &&
        mTempoEvents[mNextTempoEvent].time == mCurrTick )
    {
        SetRefresh( mTempoEvents[mNextTempoEvent].multiplier );
        ++mNextTempoEvent;
    }

    TVoiceData::iterator curr = voice_data.begin();
    TVoiceData::iterator end  = voice_data.end();
    int voice                 = 0;

    while( curr != end )
    {
        UpdateVoice( voice, *curr );
        ++curr;
        ++voice;
    }

    ++mCurrTick;

    if( mCurrTick > mTimeOfLastNote )
    {
        return false;
    }

    return true;
    //return ( mCurrTick > mTimeOfLastNote ) ? false : true;
}
//---------------------------------------------------------
void CrolPlayer::rewind( int subsong )
{
    TVoiceData::iterator curr = voice_data.begin();
    TVoiceData::iterator end  = voice_data.end();

    while( curr != end )
    {
        CVoiceData &voice = *curr;

        voice.Reset();
        ++curr;
    }

    memset(bxRegister,  0, sizeof(bxRegister) );
    memset(volumeCache, 0, sizeof(volumeCache) );

    bdRegister = 0;

    opl->init();        // initialize to melodic by default
    opl->write(1,0x20); // Enable waveform select (bit 5)

    if( rol_header->mode == 0 )
    {
        opl->write( 0xbd, 0x20 ); // select rhythm mode (bit 5)
        bdRegister = 0x20;

        SetFreq( kTomtomChannel,    24 );
        SetFreq( kSnareDrumChannel, 31 );
    }

    mNextTempoEvent = 0;
    mCurrTick       = 0;

    SetRefresh(1.0f);
}
//---------------------------------------------------------
inline float fmin( int const a, int const b )
{
    return static_cast<float>( a < b ? a : b );
}
//---------------------------------------------------------
void CrolPlayer::SetRefresh( float const multiplier )
{
    float const tickBeat = fmin(kMaxTickBeat, rol_header->ticks_per_beat);

    mRefresh =  (tickBeat*rol_header->basic_tempo*multiplier) / 60.0f;
}
//---------------------------------------------------------
float CrolPlayer::getrefresh()
{
    return mRefresh;
}
//---------------------------------------------------------
void CrolPlayer::UpdateVoice( int const voice, CVoiceData &voiceData )
{
    TNoteEvents const &nEvents = voiceData.note_events;

    if( nEvents.empty() || voiceData.mEventStatus & CVoiceData::kES_NoteEnd )
    {
        return; // no note data to process, don't bother doing anything.
    }

    TInstrumentEvents  &iEvents = voiceData.instrument_events;
    TVolumeEvents      &vEvents = voiceData.volume_events;
    TPitchEvents       &pEvents = voiceData.pitch_events;

    if( !(voiceData.mEventStatus & CVoiceData::kES_InstrEnd ) &&
        iEvents[voiceData.next_instrument_event].time == mCurrTick )
    {
        if( voiceData.next_instrument_event < iEvents.size() )
        {
            send_ins_data_to_chip( voice, iEvents[voiceData.next_instrument_event].ins_index );
            ++voiceData.next_instrument_event;
        }
        else
        {
            voiceData.mEventStatus |= CVoiceData::kES_InstrEnd;
        }
    }

    if( !(voiceData.mEventStatus & CVoiceData::kES_VolumeEnd ) &&
        vEvents[voiceData.next_volume_event].time == mCurrTick )
    {
      SVolumeEvent const &volumeEvent = vEvents[voiceData.next_volume_event];

        if(  voiceData.next_volume_event < vEvents.size() )
        {
            int const volume = (int)(63.0f*(1.0f - volumeEvent.multiplier));

            SetVolume( voice, volume );

            ++voiceData.next_volume_event; // move to next volume event
        }
        else
        {
            voiceData.mEventStatus |= CVoiceData::kES_VolumeEnd;
        }        
    }

    if( voiceData.mForceNote || voiceData.current_note_duration > voiceData.mNoteDuration-1 )
    {
        if( mCurrTick != 0 )
        {
            ++voiceData.current_note;
        }

        if( voiceData.current_note < nEvents.size() )
        {
            SNoteEvent const &noteEvent = nEvents[voiceData.current_note];

            SetNote( voice, noteEvent.number );
            voiceData.current_note_duration = 0;
            voiceData.mNoteDuration         = noteEvent.duration;
            voiceData.mForceNote            = false;
        }
        else
        {
            SetNote( voice, kSilenceNote );
            voiceData.mEventStatus |= CVoiceData::kES_NoteEnd;
            return;
        }
    }

    if( !(voiceData.mEventStatus & CVoiceData::kES_PitchEnd ) &&
        pEvents[voiceData.next_pitch_event].time == mCurrTick )
    {
        if( voiceData.next_pitch_event < pEvents.size() )
        {
            SetPitch(voice,pEvents[voiceData.next_pitch_event].variation);
            ++voiceData.next_pitch_event;
        }
        else
        {
            voiceData.mEventStatus |= CVoiceData::kES_PitchEnd;
        }
    }

    ++voiceData.current_note_duration;
}
//---------------------------------------------------------
void CrolPlayer::SetNote( int const voice, int const note )
{
    if( voice < kBassDrumChannel || rol_header->mode )
    {
        SetNoteMelodic( voice, note );
    }
    else
    {
        SetNotePercussive( voice, note );
    }
}
//---------------------------------------------------------
void CrolPlayer::SetNotePercussive( int const voice, int const note )
{
    int const bit_pos = 4-voice+kBassDrumChannel;

    bdRegister &= ~( 1<<bit_pos );
    opl->write( 0xbd, bdRegister );

    if( note != kSilenceNote )
    {
        switch( voice )
        {
            case kTomtomChannel:
                SetFreq( kSnareDrumChannel, note+7 );
            case kBassDrumChannel:
                SetFreq( voice, note );
                break;
        }

        bdRegister |= 1<<bit_pos;
        opl->write( 0xbd, bdRegister );
    }
}
//---------------------------------------------------------
void CrolPlayer::SetNoteMelodic( int const voice, int const note )
{
    opl->write( 0xb0+voice, bxRegister[voice] & ~0x20 );

    if( note != kSilenceNote )
    {
        SetFreq( voice, note, true );
    }
}
//---------------------------------------------------------
void CrolPlayer::SetPitch(int const voice, real32 const variation)
{
  pitchCache[voice] = variation;
  freqCache[voice] += (uint16)((((float)freqCache[voice])*(variation-1.0f)) / kPitchFactor);

  opl->write(0xa0+voice,freqCache[voice] & 0xff);
}
//---------------------------------------------------------
void CrolPlayer::SetFreq( int const voice, int const note, bool const keyOn )
{
    uint16 freq = kNoteTable[note%12] + ((note/12) << 10);
    freq += (uint16)((((float)freq)*(pitchCache[voice]-1.0f))/kPitchFactor);

    freqCache[voice] = freq;
    bxRegister[voice] = ((freq >> 8) & 0x1f);

    opl->write( 0xa0+voice, freq & 0xff );
    opl->write( 0xb0+voice, bxRegister[voice] | (keyOn ? 0x20 : 0x0) );
}
//---------------------------------------------------------
void CrolPlayer::SetVolume( int const voice, int const volume )
{
    volumeCache[voice] = (volumeCache[voice] &0xc0) | volume;

    int const op_offset = ( voice < kSnareDrumChannel || rol_header->mode ) ? 
                          op_table[voice]+3 : drum_table[voice-kSnareDrumChannel];

    opl->write( 0x40+op_offset, volumeCache[voice] );
}
//---------------------------------------------------------
void CrolPlayer::send_ins_data_to_chip( int const voice, int const ins_index )
{
    SRolInstrument &instrument = ins_list[ins_index].instrument;

    send_operator( voice, instrument.modulator, instrument.carrier );
}
//---------------------------------------------------------
void CrolPlayer::send_operator( int const voice, SOPL2Op const &modulator,  SOPL2Op const &carrier )
{
    if( voice < kSnareDrumChannel || rol_header->mode )
    {
        int const op_offset = op_table[voice];

        opl->write( 0x20+op_offset, modulator.ammulti  );
        opl->write( 0x40+op_offset, modulator.ksltl    );
        opl->write( 0x60+op_offset, modulator.ardr     );
        opl->write( 0x80+op_offset, modulator.slrr     );
        opl->write( 0xc0+voice    , modulator.fbc      );
        opl->write( 0xe0+op_offset, modulator.waveform );

        volumeCache[voice] = (carrier.ksltl & 0xc0) | volumeCache[voice] & 0x3f;

        opl->write( 0x23+op_offset, carrier.ammulti  );
        opl->write( 0x43+op_offset, volumeCache[voice]    );
        opl->write( 0x63+op_offset, carrier.ardr     );
        opl->write( 0x83+op_offset, carrier.slrr     );
//        opl->write( 0xc3+voice    , carrier.fbc      ); <- don't bother writing this.
        opl->write( 0xe3+op_offset, carrier.waveform );
    }
    else
    {
        int const op_offset = drum_table[voice-kSnareDrumChannel];

        volumeCache[voice] = (modulator.ksltl & 0xc0) | volumeCache[voice] & 0x3f;

        opl->write( 0x20+op_offset, modulator.ammulti  );
        opl->write( 0x40+op_offset, volumeCache[voice]      );
        opl->write( 0x60+op_offset, modulator.ardr     );
        opl->write( 0x80+op_offset, modulator.slrr     );
        opl->write( 0xc0+voice    , modulator.fbc      );
        opl->write( 0xe0+op_offset, modulator.waveform );
    }
}
//---------------------------------------------------------
void CrolPlayer::load_tempo_events( binistream *f )
{
    int16 const num_tempo_events = f->readInt( 2 );

    mTempoEvents.reserve( num_tempo_events );

    for(int i=0; i<num_tempo_events; ++i)
    {
        STempoEvent event;

        event.time       = f->readInt( 2 );
        event.multiplier = f->readFloat( binio::Single );
        mTempoEvents.push_back( event );
    }
}
//---------------------------------------------------------
bool CrolPlayer::load_voice_data( binistream *f, std::string const &bnk_filename, const CFileProvider &fp )
{
    SBnkHeader bnk_header;
    binistream *bnk_file = fp.open( bnk_filename.c_str() );

    if( bnk_file )
    {
        load_bnk_info( bnk_file, bnk_header );

        int const numVoices = rol_header->mode ? kNumMelodicVoices : kNumPercussiveVoices;

        voice_data.reserve( numVoices );
        for(int i=0; i<numVoices; ++i)
        {
            CVoiceData voice;

            load_note_events( f, voice );
            load_instrument_events( f, voice, bnk_file, bnk_header );
            load_volume_events( f, voice );
            load_pitch_events( f, voice );

            voice_data.push_back( voice );
        }

        fp.close(bnk_file);

        return true;
    }

    return false;
}
//---------------------------------------------------------
void CrolPlayer::load_note_events( binistream *f, CVoiceData &voice )
{
    f->seek( 15, binio::Add );

    int16 const time_of_last_note = f->readInt( 2 );

    if( time_of_last_note != 0 )
    {
        TNoteEvents &note_events = voice.note_events;
        int16 total_duration     = 0;

        do
        {
            SNoteEvent event;

            event.number   = f->readInt( 2 );
            event.duration = f->readInt( 2 );

            event.number += kSilenceNote; // adding -12

            note_events.push_back( event );

            total_duration += event.duration;
        } while( total_duration < time_of_last_note );

        if( time_of_last_note > mTimeOfLastNote )
        {
            mTimeOfLastNote = time_of_last_note;
        }
    }

    f->seek( 15, binio::Add );
}
//---------------------------------------------------------
void CrolPlayer::load_instrument_events( binistream *f, CVoiceData &voice,
                                         binistream *bnk_file, SBnkHeader const &bnk_header )
{
    int16 const number_of_instrument_events = f->readInt( 2 );

    TInstrumentEvents &instrument_events = voice.instrument_events;

    instrument_events.reserve( number_of_instrument_events );

    for(int i=0; i<number_of_instrument_events; ++i)
    {
        SInstrumentEvent event;
        event.time = f->readInt( 2 );
        f->readString( event.name, 9 );

	    std::string event_name = event.name;
        event.ins_index = load_rol_instrument( bnk_file, bnk_header, event_name );

        instrument_events.push_back( event );

        f->seek( 1+2, binio::Add );
    }

    f->seek( 15, binio::Add );
}
//---------------------------------------------------------
void CrolPlayer::load_volume_events( binistream *f, CVoiceData &voice )
{
    int16 const number_of_volume_events = f->readInt( 2 );

    TVolumeEvents &volume_events = voice.volume_events;

    volume_events.reserve( number_of_volume_events );

    for(int i=0; i<number_of_volume_events; ++i)
    {
        SVolumeEvent event;
        event.time       = f->readInt( 2 );
        event.multiplier = f->readFloat( binio::Single );

        volume_events.push_back( event );
    }

    f->seek( 15, binio::Add );
}
//---------------------------------------------------------
void CrolPlayer::load_pitch_events( binistream *f, CVoiceData &voice )
{
    int16 const number_of_pitch_events = f->readInt( 2 );

    TPitchEvents &pitch_events = voice.pitch_events;

    pitch_events.reserve( number_of_pitch_events );

    for(int i=0; i<number_of_pitch_events; ++i)
    {
        SPitchEvent event;
        event.time      = f->readInt( 2 );
        event.variation = f->readFloat( binio::Single );

        pitch_events.push_back( event );
    }
}
//---------------------------------------------------------
bool CrolPlayer::load_bnk_info( binistream *f, SBnkHeader &header )
{
  header.version_major = f->readInt(1);
  header.version_minor = f->readInt(1);
  f->readString( header.signature, 6 );

  header.number_of_list_entries_used  = f->readInt( 2 );
  header.total_number_of_list_entries = f->readInt( 2 );

  header.abs_offset_of_name_list = f->readInt( 4 );
  header.abs_offset_of_data      = f->readInt( 4 );

  f->seek( header.abs_offset_of_name_list, binio::Set );

  TInstrumentNames &ins_name_list = header.ins_name_list;
  ins_name_list.reserve( header.number_of_list_entries_used );

  for(int i=0; i<header.number_of_list_entries_used; ++i)
    {
      SInstrumentName instrument;

      instrument.index = f->readInt( 2 );
      instrument.record_used = f->readInt(1);
      f->readString( instrument.name, 9 );

      // printf("%s = #%d\n", instrument.name, i );

      ins_name_list.push_back( instrument );
    }

  //std::sort( ins_name_list.begin(), ins_name_list.end(), StringCompare() );

  return true;
}
//---------------------------------------------------------
int CrolPlayer::load_rol_instrument( binistream *f, SBnkHeader const &header, std::string &name )
{
    TInstrumentNames const &ins_name_list = header.ins_name_list;

    int const ins_index = get_ins_index( name );

    if( ins_index != -1 )
    {
        return ins_index;
    }

    typedef TInstrumentNames::const_iterator TInsIter;
    typedef std::pair<TInsIter, TInsIter>    TInsIterPair;

    TInsIterPair range = std::equal_range( ins_name_list.begin(), 
                                           ins_name_list.end(), 
                                           name, 
                                           StringCompare() );

    if( range.first != range.second )
    {
        int const seekOffs = header.abs_offset_of_data + (range.first->index*kSizeofDataRecord);
        f->seek( seekOffs, binio::Set );
    }

    SUsedList usedIns;
    usedIns.name = name;

    if( range.first != range.second )
    {
        read_rol_instrument( f, usedIns.instrument );
    }
    else
    {
        // set up default instrument data here
        memset( &usedIns.instrument, 0, kSizeofDataRecord );
    }
    ins_list.push_back( usedIns );

    return ins_list.size()-1;
}
//---------------------------------------------------------
int CrolPlayer::get_ins_index( std::string const &name ) const
{
    for(unsigned int i=0; i<ins_list.size(); ++i)
    {
        if( stricmp(ins_list[i].name.c_str(), name.c_str()) == 0 )
        {
            return i;
        }
    }

    return -1;
}
//---------------------------------------------------------
void CrolPlayer::read_rol_instrument( binistream *f, SRolInstrument &ins )
{
  ins.mode = f->readInt(1);
  ins.voice_number = f->readInt(1);

  read_fm_operator( f, ins.modulator );
  read_fm_operator( f, ins.carrier );

  ins.modulator.waveform = f->readInt(1);
  ins.carrier.waveform = f->readInt(1);
}
//---------------------------------------------------------
void CrolPlayer::read_fm_operator( binistream *f, SOPL2Op &opl2_op )
{
  SFMOperator fm_op;

  fm_op.key_scale_level = f->readInt(1);
  fm_op.freq_multiplier = f->readInt(1);
  fm_op.feed_back = f->readInt(1);
  fm_op.attack_rate = f->readInt(1);
  fm_op.sustain_level = f->readInt(1);
  fm_op.sustaining_sound = f->readInt(1);
  fm_op.decay_rate = f->readInt(1);
  fm_op.release_rate = f->readInt(1);
  fm_op.output_level = f->readInt(1);
  fm_op.amplitude_vibrato = f->readInt(1);
  fm_op.frequency_vibrato = f->readInt(1);
  fm_op.envelope_scaling = f->readInt(1);
  fm_op.fm_type = f->readInt(1);

  opl2_op.ammulti = fm_op.amplitude_vibrato << 7 | fm_op.frequency_vibrato << 6 | fm_op.sustaining_sound << 5 | fm_op.envelope_scaling << 4 | fm_op.freq_multiplier;
  opl2_op.ksltl   = fm_op.key_scale_level   << 6 | fm_op.output_level;
  opl2_op.ardr    = fm_op.attack_rate       << 4 | fm_op.decay_rate;
  opl2_op.slrr    = fm_op.sustain_level     << 4 | fm_op.release_rate;
  opl2_op.fbc     = fm_op.feed_back         << 1 | (fm_op.fm_type ^ 1);
}
