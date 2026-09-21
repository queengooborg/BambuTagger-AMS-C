#include "tag_parser.h"
#include <ArduinoJson.h>

void TagParser::clear(SpoolInfo& info) {
  info.present = false;
  info.uid[0] = '\0';
  info.materialType[0] = '\0';
  info.detailedType[0] = '\0';
  info.color[0] = '\0';
  info.colorHex[0] = '\0';
  info.remainingGrams = 0;
  info.totalGrams = 0;
  info.nozzleTempMin = 0;
  info.nozzleTempMax = 0;
  info.batchNumber[0] = '\0';
  info.manufacturer[0] = '\0';
  info.tagReadSuccess = false;
}

bool TagParser::parse(uint8_t* data, uint16_t length, const char* uid, SpoolInfo& info) {
  info.present = true;
  info.tagReadSuccess = false;
  strncpy(info.uid, uid, sizeof(info.uid) - 1);

  if (length < BAMBU_HEADER_SIZE) {
    strcpy(info.materialType, "Data too short");
    return false;
  }

  for (uint16_t i = 0; i + 5 < length; i++) {
    if (data[i] == '{') {
      int cl = (length - i) < 255 ? (length - i) : 254;
      char tmp[256];
      memcpy(tmp, (const char*)&data[i], cl);
      tmp[cl] = '\0';
      JsonDocument jd;
      if (deserializeJson(jd, tmp) == DeserializationError::Ok &&
          jd["protocol"].is<const char*>()) {
        const char* proto = jd["protocol"] | "";
        const char* jtype = jd["type"] | "";
        if (jtype[0])
          strncpy(info.materialType, jtype, sizeof(info.materialType) - 1);
        info.totalGrams = info.remainingGrams = jd["weight"] | 0;
        info.nozzleTempMin = jd["min_temp"] | 0;
        info.nozzleTempMax = jd["max_temp"] | 0;
        const char* ch = jd["color_hex"] | "";
        if (ch[0])
          snprintf(info.colorHex, sizeof(info.colorHex), "%sFF", ch);
        const char* br = jd["brand"] | "";
        if (br[0])
          strncpy(info.manufacturer, br, sizeof(info.manufacturer) - 1);
        snprintf(info.detailedType, sizeof(info.detailedType), "%s",
                 strcmp(proto, "openspool") == 0   ? "OpenSpool"
                 : strcmp(proto, "opentag3d") == 0 ? "OpenTag3D"
                                                   : proto);
        info.tagReadSuccess = true;
        return true;
      }
    }
  }

  if (data[0] == BAMBU_TAG_MAGIC && data[1] == BAMBU_TAG_VERSION) {
    return parseBambuTLV(data + BAMBU_HEADER_SIZE, length - BAMBU_HEADER_SIZE, info);
  }

  return parseRawNTAG(data, length, uid, info);
}

static uint32_t readU32BE(const uint8_t* d, int off) {
  return ((uint32_t)d[off] << 24) | ((uint32_t)d[off + 1] << 16) | ((uint32_t)d[off + 2] << 8) |
         d[off + 3];
}
static uint16_t readU16BE(const uint8_t* d, int off) {
  return ((uint16_t)d[off] << 8) | d[off + 1];
}

bool TagParser::parseBambuTLV(uint8_t* data, uint16_t length, SpoolInfo& info) {
  uint16_t pos = 0;
  while (pos + 2 <= length) {
    uint8_t type = data[pos];
    uint8_t len = data[pos + 1];
    pos += 2;
    if (pos + len > length)
      break;
    switch (type) {
      case BAMBU_TLV_TYPE_MATERIAL:
        if (len >= 1) {
          strncpy(info.materialType, materialName(data[pos]), sizeof(info.materialType) - 1);
        }
        break;
      case BAMBU_TLV_TYPE_COLOR:
        if (len >= 4) {
          bytesToHex(&data[pos], 3, info.colorHex);
          snprintf(info.color, sizeof(info.color), "#%s", info.colorHex);
        } else if (len >= 3) {
          bytesToHex(&data[pos], 3, info.colorHex);
          snprintf(info.color, sizeof(info.color), "#%s", info.colorHex);
        }
        break;
      case BAMBU_TLV_TYPE_WEIGHT:
        if (len >= 4) {
          info.remainingGrams = (data[pos] << 8) | data[pos + 1];
          info.totalGrams = (data[pos + 2] << 8) | data[pos + 3];
        } else if (len >= 2) {
          info.remainingGrams = (data[pos] << 8) | data[pos + 1];
        }
        break;
      case BAMBU_TLV_TYPE_BATCH:
        if (len > 0) {
          uint8_t cl = len < (sizeof(info.batchNumber) - 1) ? len : (sizeof(info.batchNumber) - 1);
          memcpy(info.batchNumber, &data[pos], cl);
          info.batchNumber[cl] = '\0';
        }
        break;
      case BAMBU_TLV_TYPE_MFG:
        if (len > 0) {
          uint8_t cl =
              len < (sizeof(info.manufacturer) - 1) ? len : (sizeof(info.manufacturer) - 1);
          memcpy(info.manufacturer, &data[pos], cl);
          info.manufacturer[cl] = '\0';
        }
        break;
      default:
        break;
    }
    pos += len;
  }
  info.tagReadSuccess = (info.materialType[0] != '\0');
  return info.tagReadSuccess;
}

bool TagParser::parseRawNTAG(uint8_t* data, uint16_t length, const char* uid, SpoolInfo& info) {
  uint16_t pos = 0;
  while (pos + 2 <= length) {
    uint8_t tlvType = data[pos];
    pos++;
    if (tlvType == 0x00 || tlvType == 0xFE)
      continue;
    if (tlvType == 0x03) {
      if (pos >= length)
        break;
      uint8_t ndefLen = data[pos];
      pos++;
      int ndefEnd = pos + ndefLen;
      if (ndefEnd > length)
        break;
      if (pos + 2 <= length) {
        uint8_t ndefFlags = data[pos];
        uint8_t typeLen = data[pos + 1];
        uint8_t payloadLen = data[pos + 2];
        pos += 3;

        if (typeLen == 2 && pos + typeLen <= length && data[pos] == 'T' && data[pos + 1] == 'a') {
          pos += typeLen + 1;
          if (payloadLen > 0 && pos + payloadLen <= length) {
            int cl = payloadLen < (int)(sizeof(info.materialType) - 1)
                         ? payloadLen
                         : (int)(sizeof(info.materialType) - 1);
            memcpy(info.materialType, &data[pos], cl);
            info.materialType[cl] = '\0';
            JsonDocument jd;
            if (deserializeJson(jd, info.materialType) == DeserializationError::Ok &&
                jd["protocol"].is<const char*>()) {
              const char* proto = jd["protocol"] | "";
              const char* jtype = jd["type"] | "";
              if (jtype[0])
                strncpy(info.materialType, jtype, sizeof(info.materialType) - 1);
              info.totalGrams = info.remainingGrams = jd["weight"] | 0;
              info.nozzleTempMin = jd["min_temp"] | 0;
              info.nozzleTempMax = jd["max_temp"] | 0;
              const char* ch = jd["color_hex"] | "";
              if (ch[0])
                snprintf(info.colorHex, sizeof(info.colorHex), "%sFF", ch);
              const char* br = jd["brand"] | "";
              if (br[0])
                strncpy(info.manufacturer, br, sizeof(info.manufacturer) - 1);
              snprintf(info.detailedType, sizeof(info.detailedType), "%s",
                       strcmp(proto, "openspool") == 0   ? "OpenSpool"
                       : strcmp(proto, "opentag3d") == 0 ? "OpenTag3D"
                                                         : proto);
            }
            info.tagReadSuccess = true;
            return true;
          }
        }

        if (typeLen == 1 && pos + typeLen <= length && data[pos] == 'U') {
          pos += typeLen;
          if (payloadLen >= 1) {
            uint8_t uriCode = data[pos];
            pos++;
            int uriLen = payloadLen - 1;
            if (uriLen < 0)
              uriLen = 0;
            int remain = length - pos;
            if (uriLen > remain)
              uriLen = remain;
            static const char* PREF[] = {"", "http://www.", "https://www.", "http://", "https://"};
            const char* prefix = (uriCode < 5) ? PREF[uriCode] : "";
            int outPos = strlen(prefix);
            if (outPos > 0 && outPos < (int)(sizeof(info.detailedType) - 1))
              memcpy(info.detailedType, prefix, outPos);
            for (int i = 0;
                 i < uriLen && pos + i < ndefEnd && outPos < (int)(sizeof(info.detailedType) - 1);
                 i++)
              info.detailedType[outPos++] = data[pos + i];
            info.detailedType[outPos] = '\0';

            const char* tagLabel = "SpoolTag";
            bool isSE = (strstr(info.detailedType, "tag.spoolease.io") ||
                         strstr(info.detailedType, "openspooltag"));
            if (isSE)
              tagLabel = "SpoolEase";

            if (isSE) {
              char seType[16] = "";
              int seWE = 0, seWF = 0;
              for (const char* p = info.detailedType; *p; p++) {
                if (*p == '&' || *p == '?' || p == info.detailedType) {
                  if (p != info.detailedType)
                    p++;
                  if (strncmp(p, "M=", 2) == 0) {
                    p += 2;
                    int n = 0;
                    while (p[n] && p[n] != '&')
                      n++;
                    if (n > 0 && n < (int)sizeof(seType)) {
                      memcpy(seType, p, n);
                      seType[n] = '\0';
                    }
                    p += n - 1;
                  } else if (strncmp(p, "CC=", 3) == 0) {
                    p += 3;
                    int n = 0;
                    while (p[n] && p[n] != '&')
                      n++;
                    if (n > 0 && n <= (int)(sizeof(info.colorHex) - 1)) {
                      memcpy(info.colorHex, p, n);
                      info.colorHex[n] = '\0';
                    }
                    p += n - 1;
                  } else if (strncmp(p, "SC=", 3) == 0) {
                    p += 3;
                    int n = 0;
                    while (p[n] && p[n] != '&')
                      n++;
                    if (n > 0 && n < (int)(sizeof(info.materialType) - 1)) {
                      memcpy(info.materialType, p, n);
                      info.materialType[n] = '\0';
                    }
                    p += n - 1;
                  } else if (strncmp(p, "B=", 2) == 0) {
                    p += 2;
                    int n = 0;
                    while (p[n] && p[n] != '&')
                      n++;
                    if (n > 0 && n < (int)(sizeof(info.manufacturer) - 1)) {
                      memcpy(info.manufacturer, p, n);
                      info.manufacturer[n] = '\0';
                    }
                    p += n - 1;
                  } else if (strncmp(p, "WE=", 3) == 0) {
                    p += 3;
                    seWE = atoi(p);
                    while (*p && *p != '&')
                      p++;
                    p--;
                  } else if (strncmp(p, "WL=", 3) == 0) {
                    p += 3;
                    info.remainingGrams = atoi(p);
                    while (*p && *p != '&')
                      p++;
                    p--;
                  } else if (strncmp(p, "WF=", 3) == 0) {
                    p += 3;
                    seWF = atoi(p);
                    while (*p && *p != '&')
                      p++;
                    p--;
                  } else if (strncmp(p, "NN=", 3) == 0) {
                    p += 3;
                    info.nozzleTempMin = atoi(p);
                    while (*p && *p != '&')
                      p++;
                    p--;
                  } else if (strncmp(p, "NX=", 3) == 0) {
                    p += 3;
                    info.nozzleTempMax = atoi(p);
                    while (*p && *p != '&')
                      p++;
                    p--;
                  }
                }
              }
              info.totalGrams = seWF - seWE;
              if (info.totalGrams < 0)
                info.totalGrams = info.remainingGrams;
              strncpy(info.detailedType, "SpoolEase", sizeof(info.detailedType) - 1);
              if (seType[0])
                strncpy(info.materialType, seType, sizeof(info.materialType) - 1);
            } else {
              strcpy(info.materialType, tagLabel);
              snprintf(info.detailedType, sizeof(info.detailedType), "%s", tagLabel);
            }
            info.tagReadSuccess = true;
            return true;
          }
        }

        if (typeLen == 21 && pos + typeLen <= length &&
            strncmp((const char*)&data[pos], "application/opentag3d", 21) == 0) {
          int ps = pos + typeLen;
          if (payloadLen >= 0x66 && ps + payloadLen <= length) {
            const uint8_t* p = &data[ps];
            uint16_t version = ((uint16_t)p[0x00] << 8) | p[0x01];
            uint8_t majorVersion = version / 1000;
            snprintf(info.detailedType, sizeof(info.detailedType), "OpenTag3D v%u.%03u",
                     majorVersion, version % 1000);

            if (majorVersion < 1 || majorVersion > 2) {
              continue;
            }

            char baseMat[6] = {0};
            memcpy(baseMat, p + 0x02, 5);
            char mods[6] = {0};
            memcpy(mods, p + 0x07, 5);
            char brand[17] = {0};
            uint8_t r, g, b;
            uint16_t wg, pt;

            if (majorVersion == 1) {
              memcpy(brand, p + 0x1B, 16);
              r = p[0x4B];
              g = p[0x4C];
              b = p[0x4D];
              wg = ((uint16_t)p[0x5E] << 8) | p[0x5F];
              pt = (uint16_t)p[0x60] * 5;
            } else {
              memcpy(brand, p + 0x0C, 16);
              r = p[0x3C];
              g = p[0x3D];
              b = p[0x3E];
              wg = ((uint16_t)p[0x9E] << 8) | p[0x9F];
              pt = (uint16_t)p[0x90] * 5;
            }

            for (int j = 4; j >= 0; j--) {
              if (baseMat[j] == ' ' || baseMat[j] == 0)
                baseMat[j] = 0;
              else
                break;
            }
            for (int j = 4; j >= 0; j--) {
              if (mods[j] == ' ' || mods[j] == 0)
                mods[j] = 0;
              else
                break;
            }
            for (int j = 15; j >= 0; j--) {
              if (brand[j] == ' ' || brand[j] == 0)
                brand[j] = 0;
              else
                break;
            }
            snprintf(info.materialType, sizeof(info.materialType), "%s%s%s", baseMat,
                     mods[0] ? "-" : "", mods[0] ? mods : "");
            snprintf(info.colorHex, sizeof(info.colorHex), "%02X%02X%02XFF", r, g, b);
            info.totalGrams = info.remainingGrams = wg;
            info.nozzleTempMin = pt;
            info.nozzleTempMax = pt + 10;
            if (brand[0])
              strncpy(info.manufacturer, brand, sizeof(info.manufacturer) - 1);

            info.tagReadSuccess = true;
            return true;
          }
        }

        int ps = pos + typeLen;
        if (payloadLen > 0 && ps + payloadLen <= length) {
          if (data[ps] == 0x03) {
            if (parseRawNTAG(&data[ps], length - ps, uid, info))
              return true;
          }
          for (int i = 0; i < payloadLen && ps + i < length; i++) {
            if (data[ps + i] == '{') {
              int cl = (payloadLen - i) < 255 ? (payloadLen - i) : 254;
              char tmp[256];
              memcpy(tmp, &data[ps + i], cl);
              tmp[cl] = '\0';
              JsonDocument jd;
              if (deserializeJson(jd, tmp) == DeserializationError::Ok &&
                  jd["protocol"].is<const char*>()) {
                const char* proto = jd["protocol"] | "";
                const char* jtype = jd["type"] | "";
                if (jtype[0])
                  strncpy(info.materialType, jtype, sizeof(info.materialType) - 1);
                info.totalGrams = info.remainingGrams = jd["weight"] | 0;
                info.nozzleTempMin = jd["min_temp"] | 0;
                info.nozzleTempMax = jd["max_temp"] | 0;
                const char* ch = jd["color_hex"] | "";
                if (ch[0])
                  snprintf(info.colorHex, sizeof(info.colorHex), "%sFF", ch);
                const char* br = jd["brand"] | "";
                if (br[0])
                  strncpy(info.manufacturer, br, sizeof(info.manufacturer) - 1);
                snprintf(info.detailedType, sizeof(info.detailedType), "%s",
                         strcmp(proto, "openspool") == 0   ? "OpenSpool"
                         : strcmp(proto, "opentag3d") == 0 ? "OpenTag3D"
                                                           : proto);
                info.tagReadSuccess = true;
                return true;
              }
              break;
            }
          }
        }
      }

      char buf[64];
      int n = length < 63 ? length : 63;
      memcpy(buf, data, n);
      buf[n] = '\0';
      if (strstr(buf, "openspool"))
        strcpy(info.materialType, "OpenSpool");
      else if (strstr(buf, "opentag3d") || strstr(buf, "OpenTag3D"))
        strcpy(info.materialType, "OpenTag3D");
      else if (strstr(buf, "tag.spoolease.io"))
        strcpy(info.materialType, "SpoolEase");
      else
        strcpy(info.materialType, "SpoolTag");
      snprintf(info.detailedType, sizeof(info.detailedType), "%s", info.materialType);
      info.tagReadSuccess = true;
      return true;
    }
    if (tlvType == 0x01 || tlvType == 0x02) {
      if (pos >= length)
        break;
      pos += data[pos] + 1;
    } else if (tlvType == 0xFD)
      break;
    else {
      if (pos >= length)
        break;
      pos += data[pos] + 1;
      break;
    }
  }
  if (!info.tagReadSuccess) {
    strcpy(info.materialType, "Unknown format");
    uint8_t copyLen = length < 8 ? length : 8;
    bytesToHex(data, copyLen, info.colorHex);
  }
  return info.tagReadSuccess;
}

const char* TagParser::materialName(uint8_t id) {
  for (size_t i = 0; i < sizeof(MATERIAL_TABLE) / sizeof(MATERIAL_TABLE[0]); i++) {
    if (MATERIAL_TABLE[i].id == id)
      return MATERIAL_TABLE[i].name;
  }
  return "Unknown Material";
}

void TagParser::bytesToHex(uint8_t* bytes, uint8_t len, char* out) {
  for (uint8_t i = 0; i < len && i < 7; i++) {
    snprintf(out + (i * 2), 3, "%02X", bytes[i]);
  }
}

void TagParser::dumpHex(uint8_t* data, uint16_t length, char* out, uint16_t outLen) {
  uint16_t pos = 0;
  for (uint16_t i = 0; i < length && pos < outLen - 4; i++) {
    pos += snprintf(out + pos, outLen - pos, "%02X ", data[i]);
    if ((i + 1) % 16 == 0 && pos < outLen - 2) {
      out[pos++] = '\n';
    }
  }
  out[pos] = '\0';
}
