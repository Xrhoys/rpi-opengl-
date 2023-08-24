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

enum demux_mp4_box_type 
{
	DEMUX_MP4_BOX_FTYP, // file type and compatibility
	DEMUX_MP4_BOX_PDIN, // progressive download information
	DEMUX_MP4_BOX_MOOV, // contianer for all the metadata
	DEMUX_MP4_BOX_MVHD, // movie header, overall declarations
	DEMUX_MP4_BOX_META, // metadata
	DEMUX_MP4_BOX_TRAK, // container for an individual track or stream
	DEMUX_MP4_BOX_TKHD, // track header, overall information about the track
	DEMUX_MP4_BOX_TREF, // track reference container
	DEMUX_MP4_BOX_TRGR, // tracking group indication
	DEMUX_MP4_BOX_EDTS, // edit list container
	DEMUX_MP4_BOX_ELST, // an edit list
	DEMUX_MP4_BOX_MDIA, // container for the media information in a track
	DEMUX_MP4_BOX_MDHD, // media hedaer, oeverall information about the media
	DEMUX_MP4_BOX_HDLR, // handler, decalres the media (handle type)
	DEMUX_MP4_BOX_ELNG, // extended language tag
	DEMUX_MP4_BOX_MINF, // media information container
	DEMUX_MP4_BOX_VMHD, // video media header, overall information (video track only)
	DEMUX_MP4_BOX_SMHD, // sound media header, overall information (sound track only)
	DEMUX_MP4_BOX_HMHD, // hint media header, overall information (hint track only)
	DEMUX_MP4_BOX_STHD, // subtitle media header, overall information (subtitle track only)
	DEMUX_MP4_BOX_NMHD, // null media header, overall information (some track only)
	DEMUX_MP4_BOX_DINF, // data information box, container
	DEMUX_MP4_BOX_DREF, // data reference box, declares sources of media data in track
	DEMUX_MP4_BOX_STBL, // sample table box, container for the time/sapce map
	DEMUX_MP4_BOX_STSD, // sample descriptions (codec types, init etc.)
	DEMUX_MP4_BOX_STTS, // (decoding) time-to-sample
	DEMUX_MP4_BOX_CTTS, // (composition) time to sample
	DEMUX_MP4_BOX_CSLG, // composition to decode timeline mapping
	DEMUX_MP4_BOX_STSC, // sample-to-chunk partial data offset information
	DEMUX_MP4_BOX_STSZ, // sample sizes (framing)
	DEMUX_MP4_BOX_STZ2, // compact sample sizes (framing)
	DEMUX_MP4_BOX_STCO, // chunk offset,partial data-offset information
	DEMUX_MP4_BOX_CO64, // 64-bit chunk offset
	DEMUX_MP4_BOX_STSS, // sync sample table
	DEMUX_MP4_BOX_STSH, // shadow sync sample table
	DEMUX_MP4_BOX_PADB, // sample padding bits
	DEMUX_MP4_BOX_STDP, // sample degradation priority
	DEMUX_MP4_BOX_SDTP, // independent and disposable samples
	DEMUX_MP4_BOX_SBGP, // sample-to-group
	DEMUX_MP4_BOX_SGPD, // sample group description
	DEMUX_MP4_BOX_SUBS, // sub-sample information
	DEMUX_MP4_BOX_SAIZ, // sample auxiliary information sizess
	DEMUX_MP4_BOX_SAIO, // sample auxiliary information offsets
	DEMUX_MP4_BOX_UDTA, // user-data
	DEMUX_MP4_BOX_MVEX, // movie extends box
	DEMUX_MP4_BOX_MEHD, // movie extends header box
	DEMUX_MP4_BOX_TREX, // track extends defaults
	DEMUX_MP4_BOX_LEVA, // level assignment
	DEMUX_MP4_BOX_MOOF, // movie fragment
	DEMUX_MP4_BOX_MFHD, // movie fragment header
	DEMUX_MP4_BOX_TRAF, // track fragment
	DEMUX_MP4_BOX_TFHD, // track fragment header
	DEMUX_MP4_BOX_TRUN, // track fragment run
	DEMUX_MP4_BOX_TFDT, // sample sample auxiliary information sizes
	DEMUX_MP4_BOX_MFRA, // movie fragment random access
	DEMUX_MP4_BOX_TFRA, // track fragment random access
	DEMUX_MP4_BOX_MFRO, // movie fragment random access offset
	DEMUX_MP4_BOX_MDAT, // media data container
	DEMUX_MP4_BOX_FREE, // free space
	DEMUX_MP4_BOX_SKIP, // free space
	DEMUX_MP4_BOX_CPRT, // copyright etc.
	DEMUX_MP4_BOX_TSEL, // track selection box
	DEMUX_MP4_BOX_STRK, // sub track box
	DEMUX_MP4_BOX_STRI, // sub track information box
	DEMUX_MP4_BOX_STRD, // sub track definition box
	DEMUX_MP4_BOX_ILOC, // item location
	DEMUX_MP4_BOX_IPRO, // item protection
	DEMUX_MP4_BOX_SINF, // protection scheme information box
	DEMUX_MP4_BOX_FRMA, // original format box
	DEMUX_MP4_BOX_SCHM, // scheme type box
	DEMUX_MP4_BOX_SCHI, // scheme information box
	DEMUX_MP4_BOX_IINF, // item information
	DEMUX_MP4_BOX_XML,  // XML container
	DEMUX_MP4_BOX_BXML, // binary XML container
	DEMUX_MP4_BOX_PITM, // primary item reference
	DEMUX_MP4_BOX_FIIN, // file delivery item information
	DEMUX_MP4_BOX_PAEN, // partition entry
	DEMUX_MP4_BOX_FIRE, // file reservoir
	DEMUX_MP4_BOX_FPAR, // file partition
	DEMUX_MP4_BOX_FECR, // FEC reservoir
	DEMUX_MP4_BOX_SEGR, // file delivery session group
	DEMUX_MP4_BOX_GITN, // group id to name
	DEMUX_MP4_BOX_IDAT, // item data
	DEMUX_MP4_BOX_IREF, // item reference
	DEMUX_MP4_BOX_MECO, // additional metadata container
	DEMUX_MP4_BOX_MERE, // metabox relation
	DEMUX_MP4_BOX_STYP, // segment type
	DEMUX_MP4_BOX_SIDX, // segment index
	DEMUX_MP4_BOX_SSIX, // subsegment index
	DEMUX_MP4_BOX_PRFT, // producer reference time
	
	DEMUX_MP4_BOX_COUNT,
};

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


struct demux_mp4_box_header
{
	// NOTE(Ecy): big-endian, network format
	// NOTE(Ecy): if size == 1, largesize is read, if size = 0, last box, so goes all the way until EOF
	u32 size;
	u32 type;
};

struct demux_mp4_box
{
	demux_mp4_box_header header;
	u64 largesize;
	char userType[16];
	
	demux_mp4_box_type type;
	char *data;
	
	// FTYP
	u32 majorBrand;
	u32 minorVersion;
	char *compatibleBrand;
};


#endif //VIDEO_DECODE_H
