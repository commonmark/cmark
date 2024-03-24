#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "cmark_ctype.h"
#include "utf8.h"

static const int8_t utf8proc_utf8class[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0};

static void encode_unknown(cmark_strbuf *buf) {
  static const uint8_t repl[] = {239, 191, 189};
  cmark_strbuf_put(buf, repl, 3);
}

static int utf8proc_charlen(const uint8_t *str, bufsize_t str_len) {
  int length, i;

  if (!str_len)
    return 0;

  length = utf8proc_utf8class[str[0]];

  if (!length)
    return -1;

  if (str_len >= 0 && (bufsize_t)length > str_len)
    return -str_len;

  for (i = 1; i < length; i++) {
    if ((str[i] & 0xC0) != 0x80)
      return -i;
  }

  return length;
}

// Validate a single UTF-8 character according to RFC 3629.
static int utf8proc_valid(const uint8_t *str, bufsize_t str_len) {
  int length = utf8proc_utf8class[str[0]];

  if (!length)
    return -1;

  if ((bufsize_t)length > str_len)
    return -str_len;

  switch (length) {
  case 2:
    if ((str[1] & 0xC0) != 0x80)
      return -1;
    if (str[0] < 0xC2) {
      // Overlong
      return -length;
    }
    break;

  case 3:
    if ((str[1] & 0xC0) != 0x80)
      return -1;
    if ((str[2] & 0xC0) != 0x80)
      return -2;
    if (str[0] == 0xE0) {
      if (str[1] < 0xA0) {
        // Overlong
        return -length;
      }
    } else if (str[0] == 0xED) {
      if (str[1] >= 0xA0) {
        // Surrogate
        return -length;
      }
    }
    break;

  case 4:
    if ((str[1] & 0xC0) != 0x80)
      return -1;
    if ((str[2] & 0xC0) != 0x80)
      return -2;
    if ((str[3] & 0xC0) != 0x80)
      return -3;
    if (str[0] == 0xF0) {
      if (str[1] < 0x90) {
        // Overlong
        return -length;
      }
    } else if (str[0] >= 0xF4) {
      if (str[0] > 0xF4 || str[1] >= 0x90) {
        // Above 0x10FFFF
        return -length;
      }
    }
    break;
  }

  return length;
}

void cmark_utf8proc_check(cmark_strbuf *ob, const uint8_t *line,
                          bufsize_t size) {
  bufsize_t i = 0;

  while (i < size) {
    bufsize_t org = i;
    int charlen = 0;

    while (i < size) {
      if (line[i] < 0x80 && line[i] != 0) {
        i++;
      } else if (line[i] >= 0x80) {
        charlen = utf8proc_valid(line + i, size - i);
        if (charlen < 0) {
          charlen = -charlen;
          break;
        }
        i += charlen;
      } else if (line[i] == 0) {
        // ASCII NUL is technically valid but rejected
        // for security reasons.
        charlen = 1;
        break;
      }
    }

    if (i > org) {
      cmark_strbuf_put(ob, line + org, i - org);
    }

    if (i >= size) {
      break;
    } else {
      // Invalid UTF-8
      encode_unknown(ob);
      i += charlen;
    }
  }
}

int cmark_utf8proc_iterate(const uint8_t *str, bufsize_t str_len,
                           int32_t *dst) {
  int length;
  int32_t uc = -1;

  *dst = -1;
  length = utf8proc_charlen(str, str_len);
  if (length < 0)
    return -1;

  switch (length) {
  case 1:
    uc = str[0];
    break;
  case 2:
    uc = ((str[0] & 0x1F) << 6) + (str[1] & 0x3F);
    if (uc < 0x80)
      uc = -1;
    break;
  case 3:
    uc = ((str[0] & 0x0F) << 12) + ((str[1] & 0x3F) << 6) + (str[2] & 0x3F);
    if (uc < 0x800 || (uc >= 0xD800 && uc < 0xE000))
      uc = -1;
    break;
  case 4:
    uc = ((str[0] & 0x07) << 18) + ((str[1] & 0x3F) << 12) +
         ((str[2] & 0x3F) << 6) + (str[3] & 0x3F);
    if (uc < 0x10000 || uc >= 0x110000)
      uc = -1;
    break;
  }

  if (uc < 0)
    return -1;

  *dst = uc;
  return length;
}

void cmark_utf8proc_encode_char(int32_t uc, cmark_strbuf *buf) {
  uint8_t dst[4];
  bufsize_t len = 0;

  assert(uc >= 0);

  if (uc < 0x80) {
    dst[0] = (uint8_t)(uc);
    len = 1;
  } else if (uc < 0x800) {
    dst[0] = (uint8_t)(0xC0 + (uc >> 6));
    dst[1] = 0x80 + (uc & 0x3F);
    len = 2;
  } else if (uc == 0xFFFF) {
    dst[0] = 0xFF;
    len = 1;
  } else if (uc == 0xFFFE) {
    dst[0] = 0xFE;
    len = 1;
  } else if (uc < 0x10000) {
    dst[0] = (uint8_t)(0xE0 + (uc >> 12));
    dst[1] = 0x80 + ((uc >> 6) & 0x3F);
    dst[2] = 0x80 + (uc & 0x3F);
    len = 3;
  } else if (uc < 0x110000) {
    dst[0] = (uint8_t)(0xF0 + (uc >> 18));
    dst[1] = 0x80 + ((uc >> 12) & 0x3F);
    dst[2] = 0x80 + ((uc >> 6) & 0x3F);
    dst[3] = 0x80 + (uc & 0x3F);
    len = 4;
  } else {
    encode_unknown(buf);
    return;
  }

  cmark_strbuf_put(buf, dst, len);
}

void cmark_utf8proc_case_fold(cmark_strbuf *dest, const uint8_t *str,
                              bufsize_t len) {
  int32_t c;

#define bufpush(x) cmark_utf8proc_encode_char(x, dest)

  while (len > 0) {
    bufsize_t char_len = cmark_utf8proc_iterate(str, len, &c);

    if (char_len >= 0) {
#include "case_fold_switch.inc"
    } else {
      encode_unknown(dest);
      char_len = -char_len;
    }

    str += char_len;
    len -= char_len;
  }
}

// matches anything in the Zs class, plus LF, CR, TAB, FF.
int cmark_utf8proc_is_space(int32_t uc) {
  return (uc == 9 || uc == 10 || uc == 12 || uc == 13 || uc == 32 ||
          uc == 160 || uc == 5760 || (uc >= 8192 && uc <= 8202) || uc == 8239 ||
          uc == 8287 || uc == 12288);
}

// matches anything in the P or S classes.
int cmark_utf8proc_is_punctuation_or_symbol(int32_t uc) {
  if (uc < 128) {
    return cmark_ispunct((char)uc);
  } else {
    return (
        uc > 128 &&
        ((uc >= 161 && uc <= 169) || (uc >= 171 && uc <= 172) ||
         (uc >= 174 && uc <= 177) || (uc == 180) || (uc >= 182 && uc <= 184) ||
         (uc == 187) || (uc == 191) || (uc == 215) || (uc == 247) ||
         (uc >= 706 && uc <= 709) || (uc >= 722 && uc <= 735) ||
         (uc >= 741 && uc <= 747) || (uc == 749) || (uc >= 751 && uc <= 767) ||
         (uc == 885) || (uc == 894) || (uc >= 900 && uc <= 901) ||
         (uc == 903) || (uc == 1014) || (uc == 1154) ||
         (uc >= 1370 && uc <= 1375) || (uc >= 1417 && uc <= 1418) ||
         (uc >= 1421 && uc <= 1423) || (uc == 1470) || (uc == 1472) ||
         (uc == 1475) || (uc == 1478) || (uc >= 1523 && uc <= 1524) ||
         (uc >= 1542 && uc <= 1551) || (uc == 1563) ||
         (uc >= 1565 && uc <= 1567) || (uc >= 1642 && uc <= 1645) ||
         (uc == 1748) || (uc == 1758) || (uc == 1769) ||
         (uc >= 1789 && uc <= 1790) || (uc >= 1792 && uc <= 1805) ||
         (uc >= 2038 && uc <= 2041) || (uc >= 2046 && uc <= 2047) ||
         (uc >= 2096 && uc <= 2110) || (uc == 2142) || (uc == 2184) ||
         (uc >= 2404 && uc <= 2405) || (uc == 2416) ||
         (uc >= 2546 && uc <= 2547) || (uc >= 2554 && uc <= 2555) ||
         (uc == 2557) || (uc == 2678) || (uc >= 2800 && uc <= 2801) ||
         (uc == 2928) || (uc >= 3059 && uc <= 3066) || (uc == 3191) ||
         (uc == 3199) || (uc == 3204) || (uc == 3407) || (uc == 3449) ||
         (uc == 3572) || (uc == 3647) || (uc == 3663) ||
         (uc >= 3674 && uc <= 3675) || (uc >= 3841 && uc <= 3863) ||
         (uc >= 3866 && uc <= 3871) || (uc == 3892) || (uc == 3894) ||
         (uc == 3896) || (uc >= 3898 && uc <= 3901) || (uc == 3973) ||
         (uc >= 4030 && uc <= 4037) || (uc >= 4039 && uc <= 4044) ||
         (uc >= 4046 && uc <= 4058) || (uc >= 4170 && uc <= 4175) ||
         (uc >= 4254 && uc <= 4255) || (uc == 4347) ||
         (uc >= 4960 && uc <= 4968) || (uc >= 5008 && uc <= 5017) ||
         (uc == 5120) || (uc >= 5741 && uc <= 5742) ||
         (uc >= 5787 && uc <= 5788) || (uc >= 5867 && uc <= 5869) ||
         (uc >= 5941 && uc <= 5942) || (uc >= 6100 && uc <= 6102) ||
         (uc >= 6104 && uc <= 6107) || (uc >= 6144 && uc <= 6154) ||
         (uc == 6464) || (uc >= 6468 && uc <= 6469) ||
         (uc >= 6622 && uc <= 6655) || (uc >= 6686 && uc <= 6687) ||
         (uc >= 6816 && uc <= 6822) || (uc >= 6824 && uc <= 6829) ||
         (uc >= 7002 && uc <= 7018) || (uc >= 7028 && uc <= 7038) ||
         (uc >= 7164 && uc <= 7167) || (uc >= 7227 && uc <= 7231) ||
         (uc >= 7294 && uc <= 7295) || (uc >= 7360 && uc <= 7367) ||
         (uc == 7379) || (uc == 8125) || (uc >= 8127 && uc <= 8129) ||
         (uc >= 8141 && uc <= 8143) || (uc >= 8157 && uc <= 8159) ||
         (uc >= 8173 && uc <= 8175) || (uc >= 8189 && uc <= 8190) ||
         (uc >= 8208 && uc <= 8231) || (uc >= 8240 && uc <= 8286) ||
         (uc >= 8314 && uc <= 8318) || (uc >= 8330 && uc <= 8334) ||
         (uc >= 8352 && uc <= 8384) || (uc >= 8448 && uc <= 8449) ||
         (uc >= 8451 && uc <= 8454) || (uc >= 8456 && uc <= 8457) ||
         (uc == 8468) || (uc >= 8470 && uc <= 8472) ||
         (uc >= 8478 && uc <= 8483) || (uc == 8485) || (uc == 8487) ||
         (uc == 8489) || (uc == 8494) || (uc >= 8506 && uc <= 8507) ||
         (uc >= 8512 && uc <= 8516) || (uc >= 8522 && uc <= 8525) ||
         (uc == 8527) || (uc >= 8586 && uc <= 8587) ||
         (uc >= 8592 && uc <= 9254) || (uc >= 9280 && uc <= 9290) ||
         (uc >= 9372 && uc <= 9449) || (uc >= 9472 && uc <= 10101) ||
         (uc >= 10132 && uc <= 11123) || (uc >= 11126 && uc <= 11157) ||
         (uc >= 11159 && uc <= 11263) || (uc >= 11493 && uc <= 11498) ||
         (uc >= 11513 && uc <= 11516) || (uc >= 11518 && uc <= 11519) ||
         (uc == 11632) || (uc >= 11776 && uc <= 11822) ||
         (uc >= 11824 && uc <= 11869) || (uc >= 11904 && uc <= 11929) ||
         (uc >= 11931 && uc <= 12019) || (uc >= 12032 && uc <= 12245) ||
         (uc >= 12272 && uc <= 12283) || (uc >= 12289 && uc <= 12292) ||
         (uc >= 12296 && uc <= 12320) || (uc == 12336) ||
         (uc >= 12342 && uc <= 12343) || (uc >= 12349 && uc <= 12351) ||
         (uc >= 12443 && uc <= 12444) || (uc == 12448) || (uc == 12539) ||
         (uc >= 12688 && uc <= 12689) || (uc >= 12694 && uc <= 12703) ||
         (uc >= 12736 && uc <= 12771) || (uc >= 12800 && uc <= 12830) ||
         (uc >= 12842 && uc <= 12871) || (uc == 12880) ||
         (uc >= 12896 && uc <= 12927) || (uc >= 12938 && uc <= 12976) ||
         (uc >= 12992 && uc <= 13311) || (uc >= 19904 && uc <= 19967) ||
         (uc >= 42128 && uc <= 42182) || (uc >= 42238 && uc <= 42239) ||
         (uc >= 42509 && uc <= 42511) || (uc == 42611) || (uc == 42622) ||
         (uc >= 42738 && uc <= 42743) || (uc >= 42752 && uc <= 42774) ||
         (uc >= 42784 && uc <= 42785) || (uc >= 42889 && uc <= 42890) ||
         (uc >= 43048 && uc <= 43051) || (uc >= 43062 && uc <= 43065) ||
         (uc >= 43124 && uc <= 43127) || (uc >= 43214 && uc <= 43215) ||
         (uc >= 43256 && uc <= 43258) || (uc == 43260) ||
         (uc >= 43310 && uc <= 43311) || (uc == 43359) ||
         (uc >= 43457 && uc <= 43469) || (uc >= 43486 && uc <= 43487) ||
         (uc >= 43612 && uc <= 43615) || (uc >= 43639 && uc <= 43641) ||
         (uc >= 43742 && uc <= 43743) || (uc >= 43760 && uc <= 43761) ||
         (uc == 43867) || (uc >= 43882 && uc <= 43883) || (uc == 44011) ||
         (uc == 64297) || (uc >= 64434 && uc <= 64450) ||
         (uc >= 64830 && uc <= 64847) || (uc == 64975) ||
         (uc >= 65020 && uc <= 65023) || (uc >= 65040 && uc <= 65049) ||
         (uc >= 65072 && uc <= 65106) || (uc >= 65108 && uc <= 65126) ||
         (uc >= 65128 && uc <= 65131) || (uc >= 65281 && uc <= 65295) ||
         (uc >= 65306 && uc <= 65312) || (uc >= 65339 && uc <= 65344) ||
         (uc >= 65371 && uc <= 65381) || (uc >= 65504 && uc <= 65510) ||
         (uc >= 65512 && uc <= 65518) || (uc >= 65532 && uc <= 65533) ||
         (uc >= 65792 && uc <= 65794) || (uc >= 65847 && uc <= 65855) ||
         (uc >= 65913 && uc <= 65929) || (uc >= 65932 && uc <= 65934) ||
         (uc >= 65936 && uc <= 65948) || (uc == 65952) ||
         (uc >= 66000 && uc <= 66044) || (uc == 66463) || (uc == 66512) ||
         (uc == 66927) || (uc == 67671) || (uc >= 67703 && uc <= 67704) ||
         (uc == 67871) || (uc == 67903) || (uc >= 68176 && uc <= 68184) ||
         (uc == 68223) || (uc == 68296) || (uc >= 68336 && uc <= 68342) ||
         (uc >= 68409 && uc <= 68415) || (uc >= 68505 && uc <= 68508) ||
         (uc == 69293) || (uc >= 69461 && uc <= 69465) ||
         (uc >= 69510 && uc <= 69513) || (uc >= 69703 && uc <= 69709) ||
         (uc >= 69819 && uc <= 69820) || (uc >= 69822 && uc <= 69825) ||
         (uc >= 69952 && uc <= 69955) || (uc >= 70004 && uc <= 70005) ||
         (uc >= 70085 && uc <= 70088) || (uc == 70093) || (uc == 70107) ||
         (uc >= 70109 && uc <= 70111) || (uc >= 70200 && uc <= 70205) ||
         (uc == 70313) || (uc >= 70731 && uc <= 70735) ||
         (uc >= 70746 && uc <= 70747) || (uc == 70749) || (uc == 70854) ||
         (uc >= 71105 && uc <= 71127) || (uc >= 71233 && uc <= 71235) ||
         (uc >= 71264 && uc <= 71276) || (uc == 71353) ||
         (uc >= 71484 && uc <= 71487) || (uc == 71739) ||
         (uc >= 72004 && uc <= 72006) || (uc == 72162) ||
         (uc >= 72255 && uc <= 72262) || (uc >= 72346 && uc <= 72348) ||
         (uc >= 72350 && uc <= 72354) || (uc >= 72448 && uc <= 72457) ||
         (uc >= 72769 && uc <= 72773) || (uc >= 72816 && uc <= 72817) ||
         (uc >= 73463 && uc <= 73464) || (uc >= 73539 && uc <= 73551) ||
         (uc >= 73685 && uc <= 73713) || (uc == 73727) ||
         (uc >= 74864 && uc <= 74868) || (uc >= 77809 && uc <= 77810) ||
         (uc >= 92782 && uc <= 92783) || (uc == 92917) ||
         (uc >= 92983 && uc <= 92991) || (uc >= 92996 && uc <= 92997) ||
         (uc >= 93847 && uc <= 93850) || (uc == 94178) || (uc == 113820) ||
         (uc == 113823) || (uc >= 118608 && uc <= 118723) ||
         (uc >= 118784 && uc <= 119029) || (uc >= 119040 && uc <= 119078) ||
         (uc >= 119081 && uc <= 119140) || (uc >= 119146 && uc <= 119148) ||
         (uc >= 119171 && uc <= 119172) || (uc >= 119180 && uc <= 119209) ||
         (uc >= 119214 && uc <= 119274) || (uc >= 119296 && uc <= 119361) ||
         (uc == 119365) || (uc >= 119552 && uc <= 119638) || (uc == 120513) ||
         (uc == 120539) || (uc == 120571) || (uc == 120597) || (uc == 120629) ||
         (uc == 120655) || (uc == 120687) || (uc == 120713) || (uc == 120745) ||
         (uc == 120771) || (uc >= 120832 && uc <= 121343) ||
         (uc >= 121399 && uc <= 121402) || (uc >= 121453 && uc <= 121460) ||
         (uc >= 121462 && uc <= 121475) || (uc >= 121477 && uc <= 121483) ||
         (uc == 123215) || (uc == 123647) || (uc >= 125278 && uc <= 125279) ||
         (uc == 126124) || (uc == 126128) || (uc == 126254) ||
         (uc >= 126704 && uc <= 126705) || (uc >= 126976 && uc <= 127019) ||
         (uc >= 127024 && uc <= 127123) || (uc >= 127136 && uc <= 127150) ||
         (uc >= 127153 && uc <= 127167) || (uc >= 127169 && uc <= 127183) ||
         (uc >= 127185 && uc <= 127221) || (uc >= 127245 && uc <= 127405) ||
         (uc >= 127462 && uc <= 127490) || (uc >= 127504 && uc <= 127547) ||
         (uc >= 127552 && uc <= 127560) || (uc >= 127568 && uc <= 127569) ||
         (uc >= 127584 && uc <= 127589) || (uc >= 127744 && uc <= 128727) ||
         (uc >= 128732 && uc <= 128748) || (uc >= 128752 && uc <= 128764) ||
         (uc >= 128768 && uc <= 128886) || (uc >= 128891 && uc <= 128985) ||
         (uc >= 128992 && uc <= 129003) || (uc == 129008) ||
         (uc >= 129024 && uc <= 129035) || (uc >= 129040 && uc <= 129095) ||
         (uc >= 129104 && uc <= 129113) || (uc >= 129120 && uc <= 129159) ||
         (uc >= 129168 && uc <= 129197) || (uc >= 129200 && uc <= 129201) ||
         (uc >= 129280 && uc <= 129619) || (uc >= 129632 && uc <= 129645) ||
         (uc >= 129648 && uc <= 129660) || (uc >= 129664 && uc <= 129672) ||
         (uc >= 129680 && uc <= 129725) || (uc >= 129727 && uc <= 129733) ||
         (uc >= 129742 && uc <= 129755) || (uc >= 129760 && uc <= 129768) ||
         (uc >= 129776 && uc <= 129784) || (uc >= 129792 && uc <= 129938) ||
         (uc >= 129940 && uc <= 129994)));
  }
}
