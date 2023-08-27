/* date = July 12th 2023 11:45 pm */

#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#ifdef __cplusplus
}
#endif

struct video_decode_frame
{
	i64 frameCount;
	i64 displayTime;
	i64 startTime;
};

struct video_decode
{
	b32               isLoaded;

	struct SwsContext *swsCtx;

	i32             streamIndex;
	AVFormatContext *formatContext;
	AVCodec         *codec;
	AVCodecContext  *codecContext;
	
	AVFrame         *pFrameRGB;
	AVFrame         *pFrame;
	AVPacket        *packet;
};

#define DEMUX_MP4_BOX_FTYP  	0x70797466 // file type and compatibility
#define DEMUX_MP4_BOX_PDIN  	0x6e696470 // progressive download information
#define DEMUX_MP4_BOX_MOOV  	0x766f6f6d // contianer for all the metadata
#define DEMUX_MP4_BOX_MVHD  	0x6468766d // movie header, overall declarations
#define DEMUX_MP4_BOX_META  	0x6174656d // metadata
#define DEMUX_MP4_BOX_TRAK  	0x6b617274 // container for an individual track or stream
#define DEMUX_MP4_BOX_TKHD  	0x64686b74 // track header, overall information about the track
#define DEMUX_MP4_BOX_TREF  	0x66657274 // track reference container
#define DEMUX_MP4_BOX_TRGR  	0x72677274 // tracking group indication
#define DEMUX_MP4_BOX_EDTS  	0x73746465 // edit list container
#define DEMUX_MP4_BOX_ELST  	0x74736c65 // an edit list
#define DEMUX_MP4_BOX_MDIA  	0x6169646d // container for the media information in a track
#define DEMUX_MP4_BOX_MDHD  	0x6468646d // media hedaer, oeverall information about the media
#define DEMUX_MP4_BOX_HDLR  	0x726c6468 // handler, decalres the media (handle type)
#define DEMUX_MP4_BOX_ELNG  	0x676e6c65 // extended language tag
#define DEMUX_MP4_BOX_MINF  	0x666e696d // media information container
#define DEMUX_MP4_BOX_VMHD  	0x64686d76 // video media header, overall information (video track only)
#define DEMUX_MP4_BOX_SMHD  	0x64686d73 // sound media header, overall information (sound track only)
#define DEMUX_MP4_BOX_HMHD  	0x64686d68 // hint media header, overall information (hint track only)
#define DEMUX_MP4_BOX_STHD  	0x64687473 // subtitle media header, overall information (subtitle track only)
#define DEMUX_MP4_BOX_NMHD  	0x64686d6e // null media header, overall information (some track only)
#define DEMUX_MP4_BOX_DINF  	0x666e6964 // data information box, container
#define DEMUX_MP4_BOX_DREF  	0x66657264 // data reference box, declares sources of media data in track
#define DEMUX_MP4_BOX_STBL  	0x6c627473 // sample table box, container for the time/sapce map
#define DEMUX_MP4_BOX_STSD  	0x64737473 // sample descriptions (codec types, init etc.)
#define DEMUX_MP4_BOX_STTS  	0x73747473 // (decoding) time-to-sample
#define DEMUX_MP4_BOX_CTTS  	0x73747463 // (composition) time to sample
#define DEMUX_MP4_BOX_CSLG  	0x676c7363 // composition to decode timeline mapping
#define DEMUX_MP4_BOX_STSC  	0x63737473 // sample-to-chunk partial data offset information
#define DEMUX_MP4_BOX_STSZ  	0x7a737473 // sample sizes (framing)
#define DEMUX_MP4_BOX_STZ2  	0x327a7473 // compact sample sizes (framing)
#define DEMUX_MP4_BOX_STCO  	0x6f637473 // chunk offset,partial data-offset information
#define DEMUX_MP4_BOX_CO64  	0x34366f63 // 64-bit chunk offset
#define DEMUX_MP4_BOX_STSS  	0x73737473 // sync sample table
#define DEMUX_MP4_BOX_STSH  	0x68737473 // shadow sync sample table
#define DEMUX_MP4_BOX_PADB  	0x62646170 // sample padding bits
#define DEMUX_MP4_BOX_STDP  	0x70647473 // sample degradation priority
#define DEMUX_MP4_BOX_SDTP  	0x70746473 // independent and disposable samples
#define DEMUX_MP4_BOX_SBGP  	0x70676273 // sample-to-group
#define DEMUX_MP4_BOX_SGPD  	0x64706773 // sample group description
#define DEMUX_MP4_BOX_SUBS  	0x73627573 // sub-sample information
#define DEMUX_MP4_BOX_SAIZ  	0x7a696173 // sample auxiliary information sizess
#define DEMUX_MP4_BOX_SAIO  	0x6f696173 // sample auxiliary information offsets
#define DEMUX_MP4_BOX_UDTA  	0x61746475 // user-data
#define DEMUX_MP4_BOX_MVEX  	0x7865766d // movie extends box
#define DEMUX_MP4_BOX_MEHD  	0x6468656d // movie extends header box
#define DEMUX_MP4_BOX_TREX  	0x78657274 // track extends defaults
#define DEMUX_MP4_BOX_LEVA  	0x6176656c // level assignment
#define DEMUX_MP4_BOX_MOOF  	0x666f6f6d // movie fragment
#define DEMUX_MP4_BOX_MFHD  	0x6468666d // movie fragment header
#define DEMUX_MP4_BOX_TRAF  	0x66617274 // track fragment
#define DEMUX_MP4_BOX_TFHD  	0x64686674 // track fragment header
#define DEMUX_MP4_BOX_TRUN  	0x6e757274 // track fragment run
#define DEMUX_MP4_BOX_TFDT  	0x74646674 // sample sample auxiliary information sizes
#define DEMUX_MP4_BOX_MFRA  	0x6172666d // movie fragment random access
#define DEMUX_MP4_BOX_TFRA  	0x61726674 // track fragment random access
#define DEMUX_MP4_BOX_MFRO  	0x6f72666d // movie fragment random access offset
#define DEMUX_MP4_BOX_MDAT  	0x7461646d // media data container
#define DEMUX_MP4_BOX_FREE  	0x65657266 // free space
#define DEMUX_MP4_BOX_SKIP  	0x70696b73 // free space
#define DEMUX_MP4_BOX_CPRT  	0x74727063 // copyright etc.
#define DEMUX_MP4_BOX_TSEL  	0x6c657374 // track selection box
#define DEMUX_MP4_BOX_STRK  	0x6b727473 // sub track box
#define DEMUX_MP4_BOX_STRI  	0x69727473 // sub track information box
#define DEMUX_MP4_BOX_STRD  	0x64727473 // sub track definition box
#define DEMUX_MP4_BOX_ILOC  	0x636f6c69 // item location
#define DEMUX_MP4_BOX_IPRO  	0x6f727069 // item protection
#define DEMUX_MP4_BOX_SINF  	0x666e6973 // protection scheme information box
#define DEMUX_MP4_BOX_FRMA  	0x616d7266 // original format box
#define DEMUX_MP4_BOX_SCHM  	0x6d686373 // scheme type box
#define DEMUX_MP4_BOX_SCHI  	0x69686373 // scheme information box
#define DEMUX_MP4_BOX_IINF  	0x666e6969 // item information
#define DEMUX_MP4_BOX_XML   	0x6c6d78   // XML container
#define DEMUX_MP4_BOX_BXML  	0x6c6d7862 // binary XML container
#define DEMUX_MP4_BOX_PITM  	0x6d746970 // primary item reference
#define DEMUX_MP4_BOX_FIIN  	0x6e696966 // file delivery item information
#define DEMUX_MP4_BOX_PAEN  	0x6e656170 // partition entry
#define DEMUX_MP4_BOX_FIRE  	0x65726966 // file reservoir
#define DEMUX_MP4_BOX_FPAR  	0x72617066 // file partition
#define DEMUX_MP4_BOX_FECR  	0x72636566 // FEC reservoir
#define DEMUX_MP4_BOX_SEGR  	0x72676573 // file delivery session group
#define DEMUX_MP4_BOX_GITN  	0x6e746967 // group id to name
#define DEMUX_MP4_BOX_IDAT  	0x74616469 // item data
#define DEMUX_MP4_BOX_IREF  	0x66657269 // item reference
#define DEMUX_MP4_BOX_MECO  	0x6f63656d // additional metadata container
#define DEMUX_MP4_BOX_MERE  	0x6572656d // metabox relation
#define DEMUX_MP4_BOX_STYP  	0x70797473 // segment type
#define DEMUX_MP4_BOX_SIDX  	0x78646973 // segment index
#define DEMUX_MP4_BOX_SSIX  	0x78697373 // subsegment index
#define DEMUX_MP4_BOX_PRFT  	0x74667270 // producer reference time

#if 0
char *demux_mp4_box_codes[DEMUX_MP4_BOX_COUNT] =
{
	"ftyp", // file type and compatibility
	"pdin", // progressive download information
	"moov", // contianer for all the metadata
	"mvhd", // movie header, overall declarations
	"meta", // metadata
	"trak", // container for an individual track or stream
	"tkhd", // track header, overall information about the track
	"tref", // track reference container
	"trgr", // tracking group indication
	"edts", // edit list container
	"elst", // an edit list
	"mdia", // container for the media information in a track
	"mdhd", // media hedaer, oeverall information about the media
	"hdlr", // handler, decalres the media (handle type)
	"elng", // extended language tag
	"minf", // media information container
	"vmhd", // video media header, overall information (video track only)
	"smhd", // sound media header, overall information (sound track only)
	"hmhd", // hint media header, overall information (hint track only)
	"sthd", // subtitle media header, overall information (subtitle track only)
	"nmhd", // null media header, overall information (some track only)
	"dinf", // data information box, container
	"dref", // data reference box, declares sources of media data in track
	"stbl", // sample table box, container for the time/sapce map
	"stsd", // sample descriptions (codec types, init etc.)
	"stts", // (decoding) time-to-sample
	"ctts", // (composition) time to sample
	"cslg", // composition to decode timeline mapping
	"stsc", // sample-to-chunk partial data offset information
	"stsz", // sample sizes (framing)
	"stz2", // compact sample sizes (framing)
	"stco", // chunk offset,partial data-offset information
	"co64", // 64-bit chunk offset
	"stss", // sync sample table
	"stsh", // shadow sync sample table
	"padb", // sample padding bits
	"stdp", // sample degradation priority
	"sdtp", // independent and disposable samples
	"sbgp", // sample-to-group
	"sgpd", // sample group description
	"subs", // sub-sample information
	"saiz", // sample auxiliary information sizess
	"saio", // sample auxiliary information offsets
	"udta", // user-data
	"mvex", // movie extends box
	"mehd", // movie extends header box
	"trex", // track extends defaults
	"leva", // level assignment
	"moof", // movie fragment
	"mfhd", // movie fragment header
	"traf", // track fragment
	"tfhd", // track fragment header
	"trun", // track fragment run
	"tfdt", // sample sample auxiliary information sizes
	"mfra", // movie fragment random access
	"tfra", // track fragment random access
	"mfro", // movie fragment random access offset
	"mdat", // media data container
	"free", // free space
	"skip", // free space
	"cprt", // copyright etc.
	"tsel", // track selection box
	"strk", // sub track box
	"stri", // sub track information box
	"strd", // sub track definition box
	"iloc", // item location
	"ipro", // item protection
	"sinf", // protection scheme information box
	"frma", // original format box
	"schm", // scheme type box
	"schi", // scheme information box
	"iinf", // item information
	"xml",  // XML container
	"bxml", // binary XML container
	"pitm", // primary item reference
	"fiin", // file delivery item information
	"paen", // partition entry
	"fire", // file reservoir
	"fpar", // file partition
	"fecr", // FEC reservoir
	"segr", // file delivery session group
	"gitn", // group id to name
	"idat", // item data
	"iref", // item reference
	"meco", // additional metadata container
	"mere", // metabox relation
	"styp", // segment type
	"sidx", // segment index
	"ssix", // subsegment index
	"prft", // producer reference time
};
#endif

struct demux_mp4_box_header
{
	// NOTE(Ecy): big-endian, network format
	// if size == 1, largesize is read, 
	// if size = 0, last box, so goes all the way until EOF
	
	// Box Header
	u32 size;
	u32 type;
	
	u64 largesize;
	char userType[16];
};

struct demux_mp4_box_full_header
{
	u32 size;
	u32 type;
	
	u64 largesize;
	char userType[16];
	
	u32 version;
	u8  flags[3];
};

struct demux_mp4_box_ftyp
{
	demux_mp4_box_header header;
	
	u32  majorBrand;
	u32  minorVersion;
	char *compatibleBrand;
	u32  brandCharSize;
};

struct demux_mp4_box_mvhd
{
	demux_mp4_box_full_header header;
		
	u64 creationTime;
	u64 modificationTime;
	u32 timescale;
	u64 duration;
	
	r32 rate;
	r32 volume; // fixed 16
	
	u16 reserved;
	u32 reserved2[2];
	u32 matrix[9];
	u32 preDefined[6];
	u32 nextTrackID;
};

struct demux_mp4_box_meta
{
	demux_mp4_box_header header;
};

struct demux_mp4_box_tkhd
{
	demux_mp4_box_full_header header;
	
	u64 creationTime;
	u64 modificationTime;
	u32 trackID;
	u32 reserved;
	u64 duration;
	
	u32 reserved2[2];
	u16 layer;
	u16 alternateGroup;
	u16 volume;
	u16 reserved3;
	u32 matrix[9];
	u32 width;
	u32 height;
};

struct demux_mp4_box_tref_box
{
	demux_mp4_box_header header;
	
	u32  *trackIDs;
	u32  trackIDCount;
};

struct demux_mp4_box_tref
{
	demux_mp4_box_header header;
	
	demux_mp4_box_tref_box *boxes;	
	u32 count;
};

struct demux_mp4_box_trgr_box
{
	demux_mp4_box_full_header header;
	u32 *trackGroupIds;
};

struct demux_mp4_box_trgr
{
	demux_mp4_box_header header;
	
	demux_mp4_box_trgr_box *boxes;
	u32 count;
};


struct demux_mp4_box_mdhd
{
	demux_mp4_box_full_header header;
	
	u64 creationTime;
	u64 modificationTime;
	u32 timescale;
	u64 duration;
	
	u16 language; // ISO-639-2/T language code
	u16 preDefined;
};

struct demux_mp4_box_hdlr
{
	demux_mp4_box_full_header header;
	
	u32  preDefined;
	u32  handlerType;
	u32  reserved[3];
	char *name;
	u32  nameSize;
};

struct demux_mp4_box_minf
{
	// ...
};

struct demux_mp4_box_mdia
{
	demux_mp4_box_mdhd mdhd;
	demux_mp4_box_minf minf;
};

struct demux_mp4_box_trak
{
	demux_mp4_box_tkhd tkhd;
	demux_mp4_box_tref tref;
	demux_mp4_box_trgr trgr;
	demux_mp4_box_mdia mdia;
	demux_mp4_box_hdlr hdlr;
};

struct demux_mp4_box_moov
{
	// MOOV
	demux_mp4_box_mvhd mvhd;
	demux_mp4_box_trak *trak;
	u32                trakCount;
	
	
	char *data;
};

#endif //VIDEO_DECODE_H
