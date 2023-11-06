#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum replacementStrategy {LFU, LRU};

struct cache {
  struct set *sets;
  int numberOfSets;
  int numberOfLines;
  int blockSize;
  int numberOfPhysicalAddressBits;
  enum replacementStrategy replacementPreference;
  int numberOfCycles;
  int numberOfMisses;
  int numberOfHits;
  int hitTime;
  int missPenalty;
};

struct line {
  uint64_t addressesWithinLine;
  int useFrequency;
  int cycleLastUsed;
  // 0 if this line was never loaded, 1 if otherwise
  short warmedUp;
};

struct set {
  struct line *lines;
};

void buildCache();
void simulate();
void getHexAddressBits(char *userInput, uint64_t *inputAddress);
void setTheCacheFree();

struct cache *cache;

int main() {
  buildCache();
  simulate();
  setTheCacheFree();
}

void buildCache() {
  cache = malloc(sizeof(struct cache));
  int numberOfSets;
  scanf("%d", &numberOfSets);
  cache->numberOfSets = numberOfSets;

  int numberOfLines;
  scanf("%d", &numberOfLines);
  cache->numberOfLines = numberOfLines;

  int blockSize;
  scanf("%d", &blockSize);
  cache->blockSize = blockSize;

  int numberOfPhysicalAddressBits;
  scanf("%d", &numberOfPhysicalAddressBits);
  cache->numberOfPhysicalAddressBits = numberOfPhysicalAddressBits;

  char replacementPolicy[4];
  scanf("%s", replacementPolicy);

  if (strcmp("LFU", replacementPolicy) == 0) {
    cache->replacementPreference = LFU;
  } else {
    cache->replacementPreference = LRU;
  }

  int hitTime;
  scanf("%d", &hitTime);
  cache->hitTime = hitTime;

  int missPenalty;
  scanf("%d", &missPenalty);
  cache->missPenalty = missPenalty;

  cache->numberOfCycles = 0;
  cache->numberOfMisses = 0;
  cache->numberOfHits = 0;

  cache->sets = malloc(sizeof(struct set) * numberOfSets);
  // Setup each set
  for (int setNum = 0; setNum < numberOfSets; setNum++) {
    cache->sets[setNum].lines = malloc(sizeof(struct line) * numberOfLines);
    // Setup lines for each set
    for (int lineNum = 0; lineNum < numberOfLines; lineNum++) {
      cache->sets[setNum].lines[lineNum].addressesWithinLine = 0;
      cache->sets[setNum].lines[lineNum].warmedUp = 0;
      cache->sets[setNum].lines[lineNum].cycleLastUsed = 0;
      cache->sets[setNum].lines[lineNum].useFrequency = 0;
    }
  }
}

  void simulate() {
    int blockBitCount = log(cache->blockSize) / log(2);
    uint64_t blockBitMask = (1 << blockBitCount) - 1;
    int setBitCount = log(cache->numberOfSets) / log(2);
    uint64_t setBitMask = ((1 << (blockBitCount + setBitCount)) - 1) ^ blockBitMask;
    uint64_t tagBitMask = 0;
    tagBitMask = ~tagBitMask;
    tagBitMask = tagBitMask ^ (setBitMask | blockBitMask);
    char userInput[9];
    scanf("%s", userInput);
    while (strcmp("-1", userInput) != 0) {
      uint64_t inputAddress;
      getHexAddressBits(userInput, &inputAddress);
      uint64_t addressBitsWithoutBlockBits = inputAddress & ~blockBitMask;
      uint64_t setBits = inputAddress & setBitMask;
      int setNum = setBits >> blockBitCount;
      for (int lineNum = 0; lineNum < cache->numberOfLines; lineNum++) {
        if (cache->sets[setNum].lines[lineNum].warmedUp == 0) {
          cache->sets[setNum].lines[lineNum].warmedUp = 1;
          cache->sets[setNum].lines[lineNum].addressesWithinLine = addressBitsWithoutBlockBits;
          cache->sets[setNum].lines[lineNum].useFrequency++;
          cache->sets[setNum].lines[lineNum].cycleLastUsed = cache->numberOfCycles;
          cache->numberOfMisses++;
          cache->numberOfCycles += cache->missPenalty + cache->hitTime;
          // print address bits and miss
          printf("%s M\n", userInput);
          break;
        } else if (cache->sets[setNum].lines[lineNum].addressesWithinLine == addressBitsWithoutBlockBits) {
          cache->sets[setNum].lines[lineNum].useFrequency++;
          cache->sets[setNum].lines[lineNum].cycleLastUsed = cache->numberOfCycles;
          cache->numberOfHits++;
          cache->numberOfCycles += cache->hitTime;
          // print address bits and hit
          printf("%s H\n", userInput);
          break;
        } else if (cache->numberOfLines - 1 == lineNum) {
          // replacement strategy
          int lineToBeReplaced = 0;
          // if LFU
          if (cache->replacementPreference == LFU) {
            for (int i = 1; i < cache->numberOfLines; i++) {
              if (cache->sets[setNum].lines[i].useFrequency < cache->sets[setNum].lines[lineToBeReplaced].useFrequency) {
                lineToBeReplaced = i;
              }
            }
          }
          // if LRU
          if (cache->replacementPreference == LRU) {
            for (int i = 1; i < cache->numberOfLines; i++) {
              if (cache->sets[setNum].lines[i].cycleLastUsed < cache->sets[setNum].lines[lineToBeReplaced].cycleLastUsed) {
                lineToBeReplaced = i;
              }
            }
          }
          // replace address
          cache->sets[setNum].lines[lineToBeReplaced].addressesWithinLine = addressBitsWithoutBlockBits;
          cache->sets[setNum].lines[lineToBeReplaced].useFrequency = 1;
          cache->sets[setNum].lines[lineToBeReplaced].cycleLastUsed = cache->numberOfCycles;
          cache->numberOfMisses++;
          cache->numberOfCycles += cache->missPenalty + cache->hitTime;
          // print address bits and miss
          printf("%s M\n", userInput);
        }
      }
      scanf("%s", userInput);
    }
    // calculate total misses percent and output as an int
    long missPercentRounded = lround((cache->numberOfMisses * 100.0) / (cache->numberOfHits + cache->numberOfMisses));
    printf("%ld ", missPercentRounded);
    // output cycle count
    printf("%d\n", cache->numberOfCycles);
  }

  void getHexAddressBits(char *userInput, uint64_t *inputAddress) {
    *inputAddress = 0;
    int bitsInHex = 4;
    int hexNumArrayOffset = 1;
    int numberOfHexNumbers = cache->numberOfPhysicalAddressBits / 4;
      for (int hexNum = 0; hexNum < numberOfHexNumbers; hexNum++) {
        int bitShiftAmount = ((numberOfHexNumbers - hexNum - hexNumArrayOffset) * bitsInHex);
        if (userInput[hexNum] == '0') {
          *inputAddress |= 0 << bitShiftAmount; 
        } else if (userInput[hexNum] == '1') {
          *inputAddress |= 1 << bitShiftAmount; 
        } else if (userInput[hexNum] == '2') {
          *inputAddress |= 2 << bitShiftAmount; 
        } else if (userInput[hexNum] == '3') {
          *inputAddress |= 3 << bitShiftAmount; 
        } else if (userInput[hexNum] == '4') {
          *inputAddress |= 4 << bitShiftAmount; 
        } else if (userInput[hexNum] == '5') {
          *inputAddress |= 5 << bitShiftAmount; 
        } else if (userInput[hexNum] == '6') {
          *inputAddress |= 6 << bitShiftAmount; 
        } else if (userInput[hexNum] == '7') {
          *inputAddress |= 7 << bitShiftAmount; 
        } else if (userInput[hexNum] == '8') {
          *inputAddress |= 8 << bitShiftAmount; 
        } else if (userInput[hexNum] == '9') {
          *inputAddress |= 9 << bitShiftAmount; 
        } else if (userInput[hexNum] == 'a' | userInput[hexNum] == 'A') {
          *inputAddress |= 10 << bitShiftAmount; 
        } else if (userInput[hexNum] == 'b' | userInput[hexNum] == 'B') {
          *inputAddress |= 11 << bitShiftAmount; 
        } else if (userInput[hexNum] == 'c' | userInput[hexNum] == 'c') {
          *inputAddress |= 12 << bitShiftAmount; 
        } else if (userInput[hexNum] == 'd' | userInput[hexNum] == 'D') {
          *inputAddress |= 13 << bitShiftAmount; 
        } else if (userInput[hexNum] == 'e' | userInput[hexNum] == 'e') {
          *inputAddress |= 14 << bitShiftAmount; 
        } else if (userInput[hexNum] == 'f' | userInput[hexNum] == 'f') {
          *inputAddress |= 15 << bitShiftAmount; 
        }
    }
  }

  // Sometimes the best thing you can do for those you love is to let them roam free.
  void setTheCacheFree() {
    for (int setNum = 0; setNum < cache->numberOfSets; setNum++) {
      free(cache->sets[setNum].lines);
    }
    free(cache->sets);
    free(cache);
  }