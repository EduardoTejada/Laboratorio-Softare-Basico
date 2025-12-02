#ifndef BASE64_H
#define BASE64_H

void base64_encode(const char *in, const unsigned long in_len, char *out);
int base64_decode(const char *in, const unsigned long in_len, char *out);

#endif /* BASE64_H */