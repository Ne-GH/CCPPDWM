/* See LICENSE file for copyright and license details. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

/*******************************************************************************
 * 分配内存并将分配的内存初始化为0，分配的内存大小为"块数 * 每块大小"
 * 参数1：分配的内存块数
 * 参数2：分配的内存块的大小
*******************************************************************************/
void* ecalloc(size_t nmemb, size_t size) {
    void *p = calloc(nmemb,size);
    if (p == nullptr) {
		die("calloc:");
    }
    return p;
}
