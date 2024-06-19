#ifndef __DICT_H
#define __DICT_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void)V)

typedef struct dictEntry {
  void *key;
  union {
    void *val;
    uint64_t u64;
    int64_t s64;
    double d;
  } v;
  struct dictEntry *next;
} dictEntry;

typedef struct dictType {
  uint64_t (*hashFunction)(const void *key);
  void *(*keyDup)(void *privdata, const void *key);
  void *(*valDup)(void *privdata, const void *obj);
  int (*keyCompare)(void *privdata, const void *key1, const void *key2);
  void (*keyDestructor)(void *privdata, void *key);
  void (*valDestructor)(void *privdata, void *obj);
  int (*expandAllowed)(size_t moreMem, double usedRatio);
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
typedef struct dictht {
  dictEntry **table;
  unsigned long size;
  unsigned long sizemask;
  unsigned long used;
} dictht;

typedef struct dict {
  dictType *type;
  void *privdata;
  dictht ht[2];
  long rehashidx; /* rehashing not in progress if rehashidx == -1 */
  int16_t
      pauserehash; /* If >0 rehashing is paused (<0 indicates coding error) */
} dict;

/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
typedef struct dictIterator {
  dict *d;
  long index;
  int table, safe;
  dictEntry *entry, *nextEntry;
  /* unsafe iterator fingerprint for misuse detection. */
  long long fingerprint;
} dictIterator;

typedef void(dictScanFunction)(void *privdata, const dictEntry *de);
typedef void(dictScanBucketFunction)(void *privdata, dictEntry **bucketref);

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE 4

/* ------------------------------- Macros ------------------------------------*/
#define dictFreeVal(d, entry)                                                  \
  if ((d)->type->valDestructor)                                                \
  (d)->type->valDestructor((d)->privdata, (entry)->v.val)

#define dictSetVal(d, entry, _val_)                                            \
  do {                                                                         \
    if ((d)->type->valDup)                                                     \
      (entry)->v.val = (d)->type->valDup((d)->privdata, _val_);                \
    else                                                                       \
      (entry)->v.val = (_val_);                                                \
  } while (0)

#define dictSetSignedIntegerVal(entry, _val_)                                  \
  do {                                                                         \
    (entry)->v.s64 = _val_;                                                    \
  } while (0)

#define dictSetUnsignedIntegerVal(entry, _val_)                                \
  do {                                                                         \
    (entry)->v.u64 = _val_;                                                    \
  } while (0)

#define dictSetDoubleVal(entry, _val_)                                         \
  do {                                                                         \
    (entry)->v.d = _val_;                                                      \
  } while (0)

#define dictFreeKey(d, entry)                                                  \
  if ((d)->type->keyDestructor)                                                \
  (d)->type->keyDestructor((d)->privdata, (entry)->key)

#define dictSetKey(d, entry, _key_)                                            \
  do {                                                                         \
    if ((d)->type->keyDup)                                                     \
      (entry)->key = (d)->type->keyDup((d)->privdata, _key_);                  \
    else                                                                       \
      (entry)->key = (_key_);                                                  \
  } while (0)

#define dictCompareKeys(d, key1, key2)                                         \
  (((d)->type->keyCompare) ? (d)->type->keyCompare((d)->privdata, key1, key2)  \
                           : (key1) == (key2))

#define dictHashKey(d, key) (d)->type->hashFunction(key)
#define dictGetKey(he) ((he)->key)
#define dictGetVal(he) ((he)->v.val)
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)
#define dictGetDoubleVal(he) ((he)->v.d)
#define dictSlots(d) ((d)->ht[0].size + (d)->ht[1].size)
#define dictSize(d) ((d)->ht[0].used + (d)->ht[1].used)
#define dictIsRehashing(d) ((d)->rehashidx != -1)
#define dictPauseRehashing(d) (d)->pauserehash++
#define dictResumeRehashing(d) (d)->pauserehash--

/* If our unsigned long type can store a 64 bit number, use a 64 bit PRNG. */
#if ULONG_MAX >= 0xffffffffffffffff
#define randomULong() ((unsigned long)genrand64_int64())
#else
#define randomULong() random()
#endif

/* API */

uint64_t siphash(const uint8_t *in, const size_t inlen, const uint8_t *k);
uint64_t siphash_nocase(const uint8_t *in, const size_t inlen,
                        const uint8_t *k);
dict *dictCreate(dictType *type, void *privDataPtr);
int dictExpand(dict *d, unsigned long size);
int dictTryExpand(dict *d, unsigned long size);
int dictAdd(dict *d, void *key, void *val);
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);
dictEntry *dictAddOrFind(dict *d, void *key);
int dictReplace(dict *d, void *key, void *val);
int dictDelete(dict *d, const void *key);
dictEntry *dictUnlink(dict *ht, const void *key);
void dictFreeUnlinkedEntry(dict *d, dictEntry *he);
void dictRelease(dict *d);
dictEntry *dictFind(dict *d, const void *key);
void *dictFetchValue(dict *d, const void *key);
int dictResize(dict *d);
dictIterator *dictGetIterator(dict *d);
dictIterator *dictGetSafeIterator(dict *d);
dictEntry *dictNext(dictIterator *iter);
void dictReleaseIterator(dictIterator *iter);
void dictGetStats(char *buf, size_t bufsize, dict *d);
uint64_t dictGenHashFunction(const void *key, int len);
uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);
void dictEmpty(dict *d, void(callback)(void *));
void dictEnableResize(void);
void dictDisableResize(void);
int dictRehash(dict *d, int n);
int dictRehashMilliseconds(dict *d, int ms);
void dictSetHashFunctionSeed(uint8_t *seed);
uint8_t *dictGetHashFunctionSeed(void);
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn,
                       dictScanBucketFunction *bucketfn, void *privdata);
uint64_t dictGetHash(dict *d, const void *key);
dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr,
                                         uint64_t hash);

/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
