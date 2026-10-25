"""Convert LTBF's TweakXL sources (.tweak + .yaml) into the JSON the game's built-in TweakDB loader reads (2.5+).

The 2.5 loader is a port of TweakXL's YAML reader to rapidjson: same "$type" / "$base" / "$value" keys, field
mutations written as an object keyed by the directive ({"!append": [...]}), struct values as objects
({"roll", "pitch", "yaw"}), and "$type" names are RTTI names ("String", "raRef:CResource", "array:CName").
It has no "$instances" templating, so templates are expanded here. Inline records are emitted as named
top-level records and referenced by ID, which avoids relying on inline-in-mutation support.

The .tweak / .yaml files stay the source of truth (TweakXL loads them on games without the built-in loader).
Only the subset of both formats LTBF uses is supported; anything else is a hard error.

usage: tweaks_to_json.py <out.json> <source files...>
"""
import json
import re
import sys
from pathlib import Path

RTTI_TYPES = {
    'string': 'String',
    'CName': 'CName',
    'bool': 'Bool',
    'float': 'Float',
    'int': 'Int32',
    'Int32': 'Int32',
    'Float': 'Float',
    'Bool': 'Bool',
    'String': 'String',
    'ResRef': 'raRef:CResource',
    'EulerAngles': 'EulerAngles',
    'Vector3': 'Vector3',
    'TweakDBID': 'TweakDBID',
}

STRUCT_FIELDS = {
    # .tweak tuple order, from TweakXL's RedReader.Values.cpp
    'EulerAngles': ('roll', 'pitch', 'yaw'),
    'Vector3': ('x', 'y', 'z'),
}


# Inline records written without a base take their type from the field's schema in TweakXL. This script has no
# schema, so the fields LTBF uses that way are listed here (record type short name, from the 2.5 scripts dump).
INLINE_RECORD_TYPES = {
    'completionEffects': 'ObjectActionEffect',  # ObjectAction_Record.CompletionEffects -> ObjectActionEffect_Record
}


class ConvertError(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------------------
# .tweak (TweakDB source) parser, subset: groups with optional base, typed/untyped flats, = / += / -=, arrays,
# tuples, inline records "{ ... } : Base".

TOKEN_RE = re.compile(r'''
    (?P<ws>\s+)
  | (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<string>"(?:[^"\\]|\\.)*")
  | (?P<number>[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?f?)
  | (?P<op>\+=|-=|[{}\[\]();,:=])
  | (?P<name>[A-Za-z_$][A-Za-z0-9_.$]*)
''', re.S | re.X)


def tokenize(text, path):
    tokens = []
    pos = 0
    while pos < len(text):
        m = TOKEN_RE.match(text, pos)
        if not m:
            line = text.count('\n', 0, pos) + 1
            raise ConvertError(f'{path}:{line}: unexpected character {text[pos]!r}')
        kind = m.lastgroup
        if kind not in ('ws', 'comment'):
            line = text.count('\n', 0, pos) + 1
            tokens.append((kind, m.group(), line))
        pos = m.end()
    return tokens


class TweakParser:
    def __init__(self, tokens, path):
        self.tokens = tokens
        self.pos = 0
        self.path = path

    def peek(self, offset=0):
        i = self.pos + offset
        return self.tokens[i] if i < len(self.tokens) else (None, None, None)

    def next(self):
        tok = self.peek()
        self.pos += 1
        return tok

    def expect(self, value):
        kind, text, line = self.next()
        if text != value:
            raise ConvertError(f'{self.path}:{line}: expected {value!r}, got {text!r}')

    def error(self, message):
        line = self.peek()[2]
        raise ConvertError(f'{self.path}:{line}: {message}')

    def parse_file(self):
        groups = []
        while self.peek()[0] is not None:
            kind, text, line = self.peek()
            if text in ('package', 'using'):
                self.error(f'"{text}" is not supported')
            groups.append(self.parse_group())
        return groups

    def parse_group(self):
        kind, name, line = self.next()
        if kind != 'name':
            raise ConvertError(f'{self.path}:{line}: expected a group name, got {name!r}')
        base = None
        if self.peek()[1] == ':':
            self.next()
            base = self.next()[1]
        self.expect('{')
        flats = self.parse_body()
        self.expect('}')
        if self.peek()[1] == ';':
            self.next()
        return {'name': name, 'base': base, 'flats': flats, 'line': line}

    def parse_body(self):
        flats = []
        while self.peek()[1] != '}':
            kind, text, line = self.peek()
            if kind != 'name':
                self.error(f'expected a property, got {text!r}')
            type_name = None
            if self.peek(1)[0] == 'name' or (self.peek(1)[1] == '[' and self.peek(2)[1] == ']'):
                type_name = self.next()[1]
                if self.peek()[1] == '[':
                    self.next()
                    self.expect(']')
                    type_name += '[]'
            kind, name, line = self.next()
            if kind != 'name':
                raise ConvertError(f'{self.path}:{line}: expected a property name, got {name!r}')
            op = self.next()[1]
            if op not in ('=', '+=', '-='):
                raise ConvertError(f'{self.path}:{line}: nested groups are not supported ({name})')
            value = self.parse_value()
            self.expect(';')
            flats.append({'type': type_name, 'name': name, 'op': op, 'value': value, 'line': line})
        return flats

    def parse_value(self):
        kind, text, line = self.peek()
        if text == '[':
            self.next()
            items = []
            while self.peek()[1] != ']':
                items.append(self.parse_value())
                if self.peek()[1] == ',':
                    self.next()
            self.expect(']')
            return ('list', items)
        if text == '(':
            self.next()
            items = []
            while self.peek()[1] != ')':
                items.append(self.parse_scalar())
                if self.peek()[1] == ',':
                    self.next()
            self.expect(')')
            return ('tuple', items)
        if text == '{':
            self.next()
            flats = self.parse_body()
            self.expect('}')
            base = None
            if self.peek()[1] == ':':
                self.next()
                base = self.next()[1]
            return ('inline', {'base': base, 'flats': flats, 'line': line})
        return self.parse_scalar()

    def parse_scalar(self):
        kind, text, line = self.next()
        if kind == 'string':
            return ('scalar', text[1:-1].replace('\\"', '"'))
        if kind == 'number':
            text = text.rstrip('f')
            return ('scalar', float(text) if re.search(r'[.eE]', text) else int(text))
        if kind == 'name':
            if text in ('true', 'false'):
                return ('scalar', text == 'true')
            return ('scalar', text)
        raise ConvertError(f'{self.path}:{line}: unexpected {text!r}')


# ---------------------------------------------------------------------------------------------------------------------
# Output builder

class Builder:
    def __init__(self):
        self.out = {}

    def put(self, key, value, where):
        if key in self.out:
            existing = self.out[key]
            # Repeated mutations of one flat ("+=" in two places) merge into one directive object.
            if isinstance(existing, dict) and isinstance(value, dict) and existing and value and \
                    all(k.startswith('!') for k in existing) and all(k.startswith('!') for k in value):
                for directive, items in value.items():
                    existing.setdefault(directive, []).extend(items)
                return
            raise ConvertError(f'{where}: {key} is defined twice')
        self.out[key] = value

    @staticmethod
    def record_header(base):
        # TweakXL resolves a base as a record type first, then as a record ID: type names have no dots.
        return {'$base': base} if '.' in base else {'$type': base}

    def typed_value(self, type_name, value, where):
        if type_name.endswith('[]'):
            element = type_name[:-2]
            if value[0] != 'list':
                raise ConvertError(f'{where}: expected an array for {type_name}')
            return [self.typed_value(element, item, where) for item in value[1]]
        if type_name in STRUCT_FIELDS:
            if value[0] != 'tuple' or len(value[1]) != len(STRUCT_FIELDS[type_name]):
                raise ConvertError(f'{where}: expected a {len(STRUCT_FIELDS[type_name])}-tuple for {type_name}')
            return {field: float(item[1]) for field, item in zip(STRUCT_FIELDS[type_name], value[1])}
        if value[0] != 'scalar':
            raise ConvertError(f'{where}: expected a scalar for {type_name}')
        if type_name in ('float', 'Float'):
            return float(value[1])
        return value[1]

    @staticmethod
    def rtti_type(type_name, where):
        array = type_name.endswith('[]')
        base = type_name[:-2] if array else type_name
        if base not in RTTI_TYPES:
            raise ConvertError(f'{where}: unsupported type {type_name}')
        return ('array:' if array else '') + RTTI_TYPES[base]

    def untyped_value(self, value, owner, prop, where, index=None):
        kind, data = value
        if kind == 'list':
            return [self.untyped_value(item, owner, prop, where, i) for i, item in enumerate(data)]
        if kind == 'inline':
            name = f'{owner}.{prop}_inline{index if index is not None else 0}'
            base = data['base'] or INLINE_RECORD_TYPES.get(prop)
            if not base:
                raise ConvertError(f'{where}: inline record in {prop} has no base; add {prop} to INLINE_RECORD_TYPES')
            self.record(name, base, data['flats'], where)
            return name
        if kind == 'tuple':
            raise ConvertError(f'{where}: a tuple needs a declared type')
        return data

    def mutation(self, op, value, owner, prop, where):
        converted = self.untyped_value(value, owner, prop, where)
        if op == '=':
            return converted
        if not isinstance(converted, list):
            raise ConvertError(f'{where}: {op} needs an array')
        return {'!append' if op == '+=' else '!remove': converted}

    def record(self, name, base, flats, where):
        """A new record (cloned from a record ID or created from a type)."""
        rec = self.record_header(base)
        self.put(name, rec, where)
        for flat in flats:
            fwhere = f'{where}:{flat["line"]}'
            if flat['type']:
                self.typed_flat(f'{name}.{flat["name"]}', flat, fwhere)
            else:
                rec[flat['name']] = self.mutation(flat['op'], flat['value'], name, flat['name'], fwhere)

    def typed_flat(self, path, flat, where):
        if flat['op'] != '=':
            raise ConvertError(f'{where}: {flat["op"]} on a typed declaration is not supported')
        self.put(path, {'$type': self.rtti_type(flat['type'], where),
                        '$value': self.typed_value(flat['type'], flat['value'], where)}, where)

    def group(self, group, path):
        where = f'{path}:{group["line"]}'
        if group['base']:
            self.record(group['name'], group['base'], group['flats'], where)
            return
        # No base: an existing record or a flat group. Either way every field is the flat "<group>.<field>".
        for flat in group['flats']:
            fwhere = f'{path}:{flat["line"]}'
            flat_path = f'{group["name"]}.{flat["name"]}'
            if flat['type']:
                self.typed_flat(flat_path, flat, fwhere)
            else:
                self.put(flat_path, self.mutation(flat['op'], flat['value'], group['name'], flat['name'], fwhere), fwhere)


# ---------------------------------------------------------------------------------------------------------------------
# YAML (TweakXL schema) subset: block maps and sequences, "!append" item tags, flow [..] / {..}, n"CName" literals,
# "$instances" templates with $(var) substitution in keys and values.

def yaml_scalar(text):
    text = text.strip()
    if text.startswith('n"') and text.endswith('"'):
        return text[2:-1]
    if len(text) >= 2 and text[0] == text[-1] and text[0] in '"\'':
        return text[1:-1]
    if text in ('true', 'false'):
        return text == 'true'
    if re.fullmatch(r'[-+]?\d+', text):
        return int(text)
    if re.fullmatch(r'[-+]?(\d+\.\d*|\.\d+)([eE][-+]?\d+)?', text):
        return float(text)
    return text


def split_flow(text):
    parts, depth, cur, quote = [], 0, '', None
    for ch in text:
        if quote:
            cur += ch
            if ch == quote:
                quote = None
            continue
        if ch in '"\'':
            quote = ch
        elif ch in '[{':
            depth += 1
        elif ch in ']}':
            depth -= 1
        elif ch == ',' and depth == 0:
            parts.append(cur)
            cur = ''
            continue
        cur += ch
    if cur.strip():
        parts.append(cur)
    return parts


def yaml_flow(text):
    text = text.strip()
    if text.startswith('['):
        return [yaml_flow(p) for p in split_flow(text[1:-1])]
    if text.startswith('{'):
        out = {}
        for part in split_flow(text[1:-1]):
            key, _, value = part.partition(':')
            out[key.strip()] = yaml_flow(value)
        return out
    return yaml_scalar(text)


def parse_yaml(text, path):
    lines = []
    for number, raw in enumerate(text.splitlines(), 1):
        stripped = raw.split(' #')[0].rstrip() if not raw.lstrip().startswith('#') else ''
        if stripped.strip():
            lines.append((len(stripped) - len(stripped.lstrip()), stripped.strip(), number))

    def block(i, indent):
        if lines[i][1].startswith('- '):
            items = []
            while i < len(lines) and lines[i][0] == indent and lines[i][1].startswith('- '):
                body = lines[i][1][2:].strip()
                tag = None
                if body.startswith('!'):
                    tag, _, body = body.partition(' ')
                    body = body.strip()
                if body and ':' in body and not body.startswith(('[', '{')):
                    # "- key: value" starts an inline map at indent + 2
                    lines[i] = (indent + 2, body, lines[i][2])
                    value, i = block(i, indent + 2)
                elif body:
                    value, i = yaml_flow(body), i + 1
                else:
                    value, i = block(i + 1, lines[i + 1][0])
                items.append((tag, value))
            return items, i
        out = {}
        while i < len(lines) and lines[i][0] == indent:
            key, _, rest = lines[i][1].partition(':')
            key, rest = key.strip(), rest.strip()
            if rest:
                out[key] = yaml_flow(rest)
                i += 1
            elif i + 1 < len(lines) and lines[i + 1][0] > indent:
                out[key], i = block(i + 1, lines[i + 1][0])
            else:
                out[key], i = None, i + 1
        if i < len(lines) and lines[i][0] > indent:
            raise ConvertError(f'{path}:{lines[i][2]}: unexpected indentation')
        return out, i

    docs = []
    i = 0
    while i < len(lines):
        if lines[i][0] != 0:
            raise ConvertError(f'{path}:{lines[i][2]}: expected a top-level key')
        # Repeated top-level keys (templates) are separate entries, so parse one key at a time.
        key, _, rest = lines[i][1].partition(':')
        start = i
        j = i + 1
        while j < len(lines) and lines[j][0] > 0:
            j += 1
        sub = lines[start + 1:j]
        lines_backup = lines
        lines = sub
        value, _ = block(0, sub[0][0]) if sub else (yaml_flow(rest), 0)
        lines = lines_backup
        docs.append((key.strip(), value, lines[start][2]))
        i = j
    return docs


def substitute(value, variables):
    if isinstance(value, str):
        for name, repl in variables.items():
            value = value.replace(f'$({name})', str(repl))
        return value
    if isinstance(value, dict):
        return {substitute(k, variables): substitute(v, variables) for k, v in value.items()}
    if isinstance(value, list):
        # Block sequences are lists of (tag, value); flow sequences are plain lists.
        return [(item[0], substitute(item[1], variables)) if isinstance(item, tuple) else substitute(item, variables)
                for item in value]
    return value


class YamlConverter:
    """Turns TweakXL YAML entries into built-in JSON, sharing inline records between template instances."""

    def __init__(self, builder):
        self.builder = builder
        self.shared = {}

    def inline(self, data, where):
        key = json.dumps(data, sort_keys=True)
        if key not in self.shared:
            type_name = data.get('$type') or data.get('$base')
            label = data.get('name', 'inline')
            name = f'LTBF.{type_name}.{label}_{len(self.shared)}'
            self.builder.put(name, data, where)
            self.shared[key] = name
        return self.shared[key]

    def convert_items(self, items, where):
        """A block sequence of (tag, value): plain list, or a {"!directive": [...]} object if tagged."""
        tags = {tag for tag, _ in items}
        values = []
        for tag, value in items:
            if isinstance(value, dict):
                value = self.inline({k: self.convert(v, where) for k, v in value.items()}, where)
            values.append(value)
        if tags == {None}:
            return values
        if len(tags) != 1:
            raise ConvertError(f'{where}: mixing tagged and untagged items is not supported')
        return {tags.pop(): values}

    def convert(self, value, where):
        if isinstance(value, list) and value and isinstance(value[0], tuple):
            return self.convert_items(value, where)
        if isinstance(value, dict):
            return {k: self.convert(v, where) for k, v in value.items()}
        return value

    def entry(self, key, value, where):
        if isinstance(value, dict) and '$instances' in value:
            instances = value['$instances']
            body = {k: v for k, v in value.items() if k != '$instances'}
            for _, variables in instances:
                self.entry(substitute(key, variables), substitute(body, variables), where)
            return
        if '$(' in key:
            raise ConvertError(f'{where}: unexpanded template key {key}')
        converted = self.convert(value, where)
        if isinstance(converted, dict) and not any(k.startswith('$') for k in converted):
            # Field updates on an existing record: emit each as its flat, like the .tweak path does.
            for prop, prop_value in converted.items():
                self.builder.put(f'{key}.{prop}', prop_value, where)
        else:
            self.builder.put(key, converted, where)


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    out_path = Path(argv[1])
    builder = Builder()
    yaml_converter = YamlConverter(builder)
    for source in sorted(Path(p) for p in argv[2:]):
        text = source.read_text(encoding='utf-8-sig')
        if source.suffix == '.tweak':
            for group in TweakParser(tokenize(text, source.name), source.name).parse_file():
                builder.group(group, source.name)
        elif source.suffix in ('.yaml', '.yml'):
            for key, value, line in parse_yaml(text, source.name):
                yaml_converter.entry(key, value, f'{source.name}:{line}')
        else:
            raise ConvertError(f'{source}: unknown tweak format')
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(builder.out, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'{out_path}: {len(builder.out)} entries')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except ConvertError as e:
        print(f'tweaks_to_json: error: {e}', file=sys.stderr)
        sys.exit(1)
