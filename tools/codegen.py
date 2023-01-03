import json
from queue import SimpleQueue

def load_class_db(filename):
    with open(filename) as f:
        class_db = json.load(f)

    for i, (name, cls) in enumerate(class_db.items()):
        cls["tid"] = i+1
        cls["children"] = []
        cls["descendants"] = []
        cls["ancestors"] = []

    # Add ancestors
    for name, cls in class_db.items():
        ancestor = name
        while ancestor:
            cls["ancestors"].append(ancestor)
            ancestor = class_db[ancestor].get("base")

    # Add children
    for name, cls in class_db.items():
        if "base" in cls:
            class_db[cls["base"]]["children"].append(name)

    # Add descendants
    for name, cls in class_db.items():
        queue = SimpleQueue()
        queue.put(name)
        while not queue.empty():
            item = queue.get()
            if item != name:
                cls["descendants"].append(item)
            for child in class_db[item]["children"]:
                queue.put(child)

    # Add descendant lookup array
    for name, cls in class_db.items():
        lookup = [False] * (len(class_db) + 1)
        lookup[cls["tid"]] = True
        for child in cls["descendants"]:
            lookup[class_db[child]["tid"]] = True
        cls["descendant_lookup_table"] = lookup
    return class_db

from jinja2 import Template
import argparse

def removesuffix(str, key):
    if str[-len(key):] == key:
        return str[:-len(key)]

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("classdb", type=str, help="classdb JSON file location")
    parser.add_argument("template", type=str, help="template file location")
    args = parser.parse_args()

    class_db = load_class_db(args.classdb)
    with open(args.template) as tmpl_f:
        template = Template(tmpl_f.read(), trim_blocks=True, lstrip_blocks=True)
        with open(removesuffix(args.template, ".jinja"), 'w') as write_f:
            rendered = template.render(class_db=class_db, descendant_lookup_table_header=[False]*(len(class_db)+1))
            write_f.write(rendered)


