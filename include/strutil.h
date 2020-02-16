#ifndef _STRUTIL_H_
#define _STRUTIL_H_


#define IN_RANGE(v, s, e)   (((s) <= (v)) && ((v) <= (e)))
#define IS_UPPER(c)         IN_RANGE(c, 'A', 'Z')
#define IS_LOWER(c)         IN_RANGE(c, 'a', 'z')
#define IS_DIGIT(c)         IN_RANGE(c, '0', '9')
#define IS_ALPHA(c)         (IS_UPPER(c) || IS_LOWER(c))
#define IS_ALNUM(c)         (IS_DIGIT(c) || IS_ALPHA(c))
#define IS_SPACE(c)         ((c) == ' ')
#define IS_TAB(c)           ((c) == '\t')
#define IS_NEWLINE(c)       ((c) == '\n')
#define IS_PRINT(c)         (IS_SPACE(c) || IS_TAB(c) || IS_NEWLINE(c) || IS_ALNUM(c))


uint32_t strip(char * Copy_pcStr, uint32_t Copy_u32StrLen);

#endif /*   _STRUTIL_H_ */