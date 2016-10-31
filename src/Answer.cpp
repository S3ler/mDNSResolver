#include "Answer.h"
#include <stdlib.h>

namespace mDNSResolver {
  Answer::Answer() {}

  MDNS_RESULT Answer::parse(unsigned char* buffer, unsigned int len) {
    if((buffer[2] & 0b10000000) != 0b10000000) {
      // Not an answer packet
      return E_MDNS_OK;
    }

    if(buffer[2] & 0b00000010) {
      // Truncated - we don't know what to do with these
      return E_MDNS_TRUNCATED;
    }

    if (buffer[3] & 0b00001111) {
      return E_MDNS_PACKET_ERROR;
    }

    unsigned int answerCount = (buffer[6] << 8) + buffer[7];

    // For this library, we are only interested in packets that contain answers
    if(answerCount == 0) {
      return E_MDNS_OK;
    }

    unsigned int offset = 0;

    MDNS_RESULT questionResult = skipQuestions(buffer, len, &offset);
    if(questionResult != E_MDNS_OK) {
      return questionResult;
    }

    //for(int i = 0; i < answerCount; i++) {
      //Answer answer();

      //if(parseAnswer(buffer, len, &offset, &answer) == E_MDNS_OK) {
        //int resultIndex = search(answer.name);
        //if(resultIndex != -1) {
          //if(answer.type == 0x01) {
            //_cache[resultIndex].ipAddress = IPAddress((int)answer.data[0], (int)answer.data[1], (int)answer.data[2], (int)answer.data[3]);
            //_cache[resultIndex].ttl = answer.ttl;
            //_cache[resultIndex].waiting = false;
          //} else if(answer.type == 0x05) {
            //// If data is already in there, copy the data
            //int cnameIndex = search((char *)answer.data);

            //if(cnameIndex != -1) {
              //_cache[resultIndex].ipAddress = _cache[cnameIndex].ipAddress;
              //_cache[resultIndex].waiting = false;
              //_cache[resultIndex].ttl = answer.ttl;
            //} else {
              //Response r = buildResponse((char *)answer.data);
              //insert(r);
            //}
          //}
        //}

        //free(answer.data);
        //free(answer.name);
        //return E_MDNS_OK;
      //} else {
        //if(answer.data) {
          //free(answer.data);
        //}
        //if(answer.name) {
          //free(answer.name);
        //}
        //return E_MDNS_PARSING_ERROR;
      //}
    //}
  }

  // Converts a encoded DNS name into a FQDN.
  // name: pointer to char array where the result will be stored. Needs to have already been allocated. It's allocated length should be len - 1
  // mapped: The encoded DNS name
  // len: Length of mapped
  MDNS_RESULT Answer::parseName(char** name, const char* mapped, unsigned int len) {
    unsigned int namePointer = 0;
    unsigned int mapPointer = 0;

    while(mapPointer < len) {
      int labelLength = mapped[mapPointer++];

      if(namePointer + labelLength > len - 1) {
        return E_MDNS_INVALID_LABEL_LENGTH;
      }

      if(namePointer != 0) {
        (*name)[namePointer++] = '.';
      }

      for(int i = 0; i < labelLength; i++) {
        (*name)[namePointer++] = mapped[mapPointer++];
      }
    }

    (*name)[len - 1] = '\0';

    return E_MDNS_OK;
  }

  int Answer::assembleName(unsigned char *buffer, unsigned int len, unsigned int *offset, char **name, unsigned int maxlen) {
    unsigned int index = 0;

    while(buffer[*offset] != '\0' && index < maxlen) {
      if((buffer[*offset] & 0xc0) == 0xc0) {
        char *pointer;
        unsigned int pointerOffset = ((buffer[(*offset)++] & 0x3f) << 8) + buffer[*offset];
        if(pointerOffset > len) {
          // Points to somewhere beyond the packet
          return -1 * E_MDNS_POINTER_OVERFLOW;
        }
        assembleName(buffer, len, &pointerOffset, &pointer);

        unsigned int pointerLen = strlen(pointer);
        memcpy(*name + index, pointer, pointerLen);

        index += pointerLen;
        free(pointer);

        break;
      } else {
        (*name)[index++] = buffer[(*offset)++];
      }
    }

    (*name)[index++] = '\0';
    (*offset)++;
    return index;
  }

  int Answer::assembleName(unsigned char *buffer, unsigned int len, unsigned int *offset, char **name) {
    return assembleName(buffer, len, offset, name, MDNS_MAX_NAME_LEN);
  }

  // Work out how many bytes are dedicated to questions. Since we aren't answering questions, they can be skipped
  // buffer: The mDNS packet we are parsing
  // len: Length of the packet
  // offset: the byte we are up to in the parsing process
  MDNS_RESULT Answer::skipQuestions(unsigned char* buffer, unsigned int len, unsigned int* offset) {
    unsigned int questionCount = (buffer[4] << 8) + buffer[5];

    *offset += 12;
    for(int i = 0; i < questionCount; i++) {

      while(buffer[*offset] != '\0') {
        // If it's a pointer, add two to the counter
        if((buffer[*offset] & 0xc0) == 0xc0) {
          (*offset) += 2;
          break;
        } else {
          unsigned int labelSize = (unsigned int)buffer[*offset];

          if(labelSize > 0x3f) {
            return E_MDNS_PACKET_ERROR;
          }

          (*offset) += 1; // Increment to move to the next byte
          (*offset) += labelSize;

          if(*offset > len) {
            return E_MDNS_PACKET_ERROR;
          }
        }
      }

      (*offset) += 5; // 2 bytes for the qtypes and 2 bytes qclass + plus one to land us on the next bit
    }

    if(*offset > len + 1) {
      return E_MDNS_PACKET_ERROR;
    }

    return E_MDNS_OK;
  }
};
