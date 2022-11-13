/**
 *
 * Copyright (c) 2022, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of RNP sexp library
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other matrials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Original copyright
 *
 * SEXP standard header file: sexp.h
 * Ronald L. Rivest
 * 6/29/1997
 */

#pragma once

#include <algorithm>
#include <fstream>
#include <vector>

namespace sexp {

/*
 * SEXP octet definitions
 */

class sexpCharDefs {
  protected:
    static char upper[256];       /* upper[c] is upper case version of c */
    static bool whitespace[256];  /* whitespace[c] is nonzero if c is whitespace */
    static bool decdigit[256];    /* decdigit[c] is nonzero if c is a dec digit */
    static char decvalue[256];    /* decvalue[c] is value of c as dec digit */
    static bool hexdigit[256];    /* hexdigit[c] is nonzero if c is a hex digit */
    static char hexvalue[256];    /* hexvalue[c] is value of c as a hex digit */
    static bool base64digit[256]; /* base64char[c] is nonzero if c is base64 digit */
    static char base64value[256]; /* base64value[c] is value of c as base64 digit */
    static bool tokenchar[256];   /* tokenchar[c] is true if c can be in a token */
    static bool alpha[256];       /* alpha[c] is true if c is alphabetic A-Z a-z */

    static bool initialized;
    static void initialize_character_tables(void);
    static bool is_white_space(int c);
    static bool is_dec_digit(int c);
    static bool is_hex_digit(int c);
    static bool is_base64_digit(int c);
    static bool is_token_char(int c);
    static bool is_alpha(int c);

    sexpCharDefs(void)
    {
        if (!initialized) {
            initialize_character_tables();
            initialized = true;
        }
    }
};

/*
 * SEXP simple string
 */

class sexpOutputStream;

typedef unsigned char octet;

class sexpSimpleString : public std::basic_string<octet>, private sexpCharDefs {
  public:
    sexpSimpleString &append(int c)
    {
        (*this) += (octet)(c & 0xFF);
        return *this;
    }
    // Returns length for printing simple string as a token
    uint32_t advanced_length_token(void) const { return length(); }
    // Returns length for printing simple string as a base64 string
    uint32_t advanced_length_base64(void) const { return (2 + 4 * ((length() + 2) / 3)); }
    // Returns length for printing simple string ss in quoted-string mode
    uint32_t advanced_length_quoted(void) const { return (1 + length() + 1); }
    // Returns length for printing simple string ss in hexadecimal mode
    uint32_t advanced_length_hexadecimal(void) const { return (1 + 2 * length() + 1); }
    uint32_t advanced_length(sexpOutputStream *os) const;

    sexpOutputStream *print_canonical_verbatim(sexpOutputStream *os) const;
    sexpOutputStream *print_advanced(sexpOutputStream *os) const;
    sexpOutputStream *print_token(sexpOutputStream *os) const;
    sexpOutputStream *print_verbatim(sexpOutputStream *os) const;
    sexpOutputStream *print_quoted(sexpOutputStream *os) const;
    sexpOutputStream *print_hexadecimal(sexpOutputStream *os) const;
    sexpOutputStream *print_base64(sexpOutputStream *os) const;

    bool can_print_as_quoted_string(void) const;
    bool can_print_as_token(const sexpOutputStream *os) const;
};

/*
 * SEXP object
 */

class sexpObject {
  public:
    virtual ~sexpObject(){};

    virtual sexpOutputStream *print_canonical(sexpOutputStream *os) const = 0;
    virtual sexpOutputStream *print_advanced(sexpOutputStream *os) const;
    virtual uint32_t          advanced_length(sexpOutputStream *os) const = 0;
};

/*
 * SEXP string
 */

class sexpString : public sexpObject {
  protected:
    sexpSimpleString *presentation_hint;
    sexpSimpleString *string;

  public:
    sexpString(void) : presentation_hint(NULL), string(NULL) {}

    virtual ~sexpString()
    {
        if (presentation_hint)
            delete presentation_hint;
        if (string)
            delete string;
    }

    sexpSimpleString *get_string(void) const { return string; }
    sexpSimpleString *set_string(sexpSimpleString *ss) { return string = ss; }
    sexpSimpleString *get_presentation_hint(void) const { return presentation_hint; }
    sexpSimpleString *set_presentation_hint(sexpSimpleString *ph)
    {
        return presentation_hint = ph;
    }

    virtual sexpOutputStream *print_canonical(sexpOutputStream *os) const;
    virtual sexpOutputStream *print_advanced(sexpOutputStream *os) const;
    virtual uint32_t          advanced_length(sexpOutputStream *os) const;
};

/*
 * SEXP list
 */

class sexpList : public sexpObject, public std::vector<sexpObject *> {
  public:
    virtual ~sexpList()
    {
        sexpObject *pd;
        for (auto it = begin(); it != end(); ++it) {
            pd = *it;
            delete pd;
        }
    }

    virtual sexpOutputStream *print_canonical(sexpOutputStream *os) const;
    virtual sexpOutputStream *print_advanced(sexpOutputStream *os) const;
    virtual uint32_t          advanced_length(sexpOutputStream *os) const;
};

/*
 * SEXP input stream
 */

class sexpInputStream : private sexpCharDefs {
  protected:
    std::istream *inputFile;
    uint32_t      byte_size; /* 4 or 6 or 8 == currently scanning mode */
    int           next_char; /* character currently being scanned */
    uint32_t      bits;      /* Bits waiting to be used */
    uint32_t      n_bits;    /* number of such bits waiting to be used */
    int           count;     /* number of 8-bit characters output by get_char */
  public:
    sexpInputStream(std::istream *i);
    void             set_input(std::istream *i) { inputFile = i; }
    sexpInputStream *set_byte_size(uint32_t new_byte_size);
    uint32_t         get_byte_size(void) { return byte_size; }
    sexpInputStream *get_char(void);
    sexpInputStream *skip_white_space(void);
    sexpInputStream *skip_char(int c);

    sexpObject *      scan_to_eof();
    sexpObject *      scan_object(void);
    sexpString *      scan_string(void);
    sexpList *        scan_list(void);
    sexpSimpleString *scan_simple_string(void);
    void              scan_token(sexpSimpleString *ss);
    void              scan_verbatim_string(sexpSimpleString *ss, int length);
    void              scan_quoted_string(sexpSimpleString *ss, int length);
    void              scan_hexadecimal_string(sexpSimpleString *ss, int length);
    void              scan_base64_string(sexpSimpleString *ss, int length);
    int               scan_decimal_string(void);

    int get_next_char(void) const { return next_char; }
    int set_next_char(int c) { return next_char = c; }
};

/*
 * SEXP output stream
 */

class sexpOutputStream {
  public:
    const uint32_t default_line_length = 75;
    enum sexpPrintMode {                /* PRINTING MODES */
                         canonical = 1, /* standard for hashing and tranmission */
                         base64 = 2,    /* base64 version of canonical */
                         advanced = 3   /* pretty-printed */
    };

  protected:
    std::ostream *output_file;
    uint32_t      base64_count; /* number of hex or base64 chars printed this region */
    uint32_t      byte_size;    /* 4 or 6 or 8 depending on output mode */
    uint32_t      bits;         /* bits waiting to go out */
    uint32_t      n_bits;       /* number of bits waiting to go out */
    sexpPrintMode mode;         /* base64, advanced, or canonical */
    uint32_t      column;       /* column where next character will go */
    uint32_t      max_column;   /* max usable column, or 0 if no maximum */
    uint32_t      indent;       /* current indentation level (starts at 0) */
  public:
    sexpOutputStream(std::ostream *o);
    void              set_output(std::ostream *o) { output_file = o; }
    sexpOutputStream *put_char(int c);              /* output a character */
    sexpOutputStream *new_line(sexpPrintMode mode); /* go to next line (and indent) */
    sexpOutputStream *var_put_char(int c);
    sexpOutputStream *flush(void);
    sexpOutputStream *print_decimal(uint32_t n);

    sexpOutputStream *change_output_byte_size(int newByteSize, sexpPrintMode mode);

    sexpOutputStream *print_canonical(const sexpObject *obj)
    {
        return obj->print_canonical(this);
    }
    sexpOutputStream *print_advanced(const sexpObject *obj)
    {
        return obj->print_advanced(this);
    };
    sexpOutputStream *print_base64(const sexpObject *obj);
    sexpOutputStream *print_canonical(const sexpSimpleString *ss)
    {
        return ss->print_canonical_verbatim(this);
    }
    sexpOutputStream *print_advanced(const sexpSimpleString *ss)
    {
        return ss->print_advanced(this);
    };

    uint32_t          get_byte_size(void) const { return byte_size; }
    uint32_t          get_column(void) const { return column; }
    uint32_t          reset_column(void) { return column = 0; }
    uint32_t          get_max_column(void) const { return max_column; }
    uint32_t          set_max_column(uint32_t mc) { return max_column = mc; }
    sexpOutputStream *inc_indent(void)
    {
        ++indent;
        return this;
    }
    sexpOutputStream *dec_indent(void)
    {
        --indent;
        return this;
    }
};

} // namespace sexp
