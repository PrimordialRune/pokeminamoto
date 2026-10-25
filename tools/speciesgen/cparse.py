"""Small helpers for reading the C initializer tables that speciesgen replaced.

Only extract.py uses them (the one-time conversion from the old hand-written
tables). They understand just enough C for those files: comments, string
literals, nested braces/parentheses and designated initializers.
"""
import re
import subprocess


def strip_comments(text):
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == '\\' else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith('//', i):
            j = text.find('\n', i)
            i = n if j < 0 else j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            out.append(' ')
            i = n if j < 0 else j + 2
        else:
            out.append(c)
            i += 1
    return ''.join(out).replace('﻿', '')


def matching(text, i):
    """Index of the bracket closing the one at text[i]."""
    pairs = {'{': '}', '(': ')', '[': ']'}
    stack = []
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == '\\' else 1
            i = j + 1
            continue
        if c in pairs:
            stack.append(pairs[c])
        elif c in ')}]':
            if stack.pop() != c:
                raise ValueError('unbalanced at %d' % i)
            if not stack:
                return i
        i += 1
    raise ValueError('unclosed bracket')


def split_top(inner, sep=','):
    """Split on sep at bracket depth 0."""
    parts, depth, cur, i, n = [], 0, [], 0, len(inner)
    while i < n:
        c = inner[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and inner[j] != c:
                j += 2 if inner[j] == '\\' else 1
            cur.append(inner[i:j + 1])
            i = j + 1
            continue
        if c in '({[':
            depth += 1
        elif c in ')}]':
            depth -= 1
        if c == sep and depth == 0:
            parts.append(''.join(cur).strip())
            cur = []
        else:
            cur.append(c)
        i += 1
    last = ''.join(cur).strip()
    if last:
        parts.append(last)
    return parts


def array_body(text, name):
    """Inner text of the brace initializer of array `name`."""
    m = re.search(r'\b%s\s*(\[[^\]]*\]\s*)*=\s*\{' % re.escape(name), text)
    if not m:
        raise KeyError(name)
    start = m.end() - 1
    return text[start + 1:matching(text, start)]


def designated(inner):
    """[KEY] = VALUE entries of an initializer, in order."""
    out = []
    for part in split_top(inner):
        m = re.match(r'\[([^\]]+)\]\s*=\s*(.*)$', part, re.S)
        if not m:
            raise ValueError('not designated: ' + part[:60])
        out.append((m.group(1).strip(), m.group(2).strip()))
    return out


def fields(value):
    """.name = value entries of a struct initializer {...}."""
    value = value.strip()
    assert value[0] == '{' and value[-1] == '}', value[:40]
    out = {}
    for part in split_top(value[1:-1]):
        m = re.match(r'\.(\w+)\s*=\s*(.*)$', part, re.S)
        if not m:
            raise ValueError('not a field: ' + part[:60])
        out[m.group(1)] = m.group(2).strip()
    return out


def c_string(expr):
    """The text of _("...") "..." string concatenations, escapes kept."""
    m = re.match(r'_\((.*)\)$', expr.strip(), re.S)
    if m:
        expr = m.group(1)
    return ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', expr))


def cpp_values(names, includes, defines=()):
    """Evaluate integer macros with the C preprocessor."""
    src = ''.join('#include "%s"\n' % h for h in includes)
    src += ''.join('"@%s" = (%s)\n' % (n, n) for n in names)
    cmd = ['cpp', '-P', '-iquote', 'include', '-DMODERN=1'] + ['-D' + d for d in defines]
    out = subprocess.run(cmd, input=src, capture_output=True, text=True, check=True).stdout
    vals = {}
    for line in out.splitlines():
        m = re.match(r'"@(\w+)" = (.*)$', line)
        if m:
            vals[m.group(1)] = eval_c(m.group(2))
    return vals


def eval_c(expr):
    e = expr.replace('/', '//').replace('&&', ' and ').replace('||', ' or ')
    e = re.sub(r'\b(\d+)[uUlL]+\b', r'\1', e)
    e = re.sub(r'\(\s*(u8|u16|u32|s8|s16|s32|int)\s*\)', '', e)
    return int(eval(e, {}, {}))


def enum_values(text, first_name):
    """name -> value for the enum containing first_name (plain or = n entries)."""
    i = text.index(first_name)
    start = text.rindex('{', 0, i)
    inner = text[start + 1:matching(text, start)]
    vals, cur = {}, -1
    for part in split_top(inner):
        if '=' in part:
            k, v = [x.strip() for x in part.split('=', 1)]
            cur = eval_c(re.sub(r'\b[A-Z_][A-Z0-9_]*\b', lambda m: str(vals[m.group(0)]), v))
        else:
            k = part
            cur += 1
        vals[k] = cur
    return vals
