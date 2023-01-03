local function dump(o) {
    switch (typeof(o)) {
        case "table":
            local table = "";
            foreach (k, v in o) {
                if (table != "") {
                    table += ", "
                }
                table += dump(k) + ": " + dump(v);
            }
            return "{" + table + "}";
        case "string":
            return "'" + o + "'";
        case "integer":
            return o;
        case "bool":
            return o ? "true" : "false";
        case "array":
            local array = "";
            foreach (v in o) {
                if (array != "") {
                    array += ", ";
                }
                array += dump(v);
            }
            return "[" + array + "]";
        default:
            return typeof(o);
    }
}

return dump