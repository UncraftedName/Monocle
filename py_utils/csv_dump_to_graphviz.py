import pandas as pd
import os
import numpy as np
import argparse

DF_FLOAT_COL = "float_val"
DF_DOUBLE_COL = "double_val"

HIGH_DIFF_COL = "red"


def bool_to_gv_col(is_blue: bool):
    return "cornflowerblue" if is_blue else "orange"


def bool_to_portal_col(is_blue: bool):
    return "blue" if is_blue else "orange"


def opposite_portal_col(col_str: str):
    return "orange" if col_str == "blue" else "blue"


# rectangular table with no entries that span multiple rows/columns
class HtmlMatrix:
    def __init__(self, n_rows, n_cols):
        self._str_entries = [[""] * n_cols for _ in range(n_rows)]
        self.high_err_set = set()  # has tuples (r, c) for entries which should be colored HIGH_DIFF_COL

    def build(self) -> str:
        out_str = "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"0\">"
        for r, row in enumerate(self._str_entries):
            out_str += "<TR>"
            for c, entry in enumerate(row):
                is_high_err = (r, c) in self.high_err_set
                out_str += "<TD>"
                if is_high_err:
                    out_str += f"<FONT COLOR=\"{HIGH_DIFF_COL}\">"
                out_str += str(entry)
                if is_high_err:
                    out_str += "</FONT>"
                out_str += "</TD>"
            out_str += "</TR>"
        out_str += "</TABLE>"
        return out_str

    def dims(self):
        return (len(self._str_entries), len(self._str_entries[0]))

    def __str__(self):
        return self.build()

    def set_entry(self, r, c, x: str):
        self._str_entries[r][c] = x

    def set_entry_high_err(self, r, c):
        self.high_err_set.add((r, c))

    # create 2x1 table with title & content
    @classmethod
    def create_titled_table(cls, title: str, content: str):
        hm = cls(2, 1)
        hm._str_entries[0][0] = f"<b>{title}</b>"
        hm._str_entries[1][0] = content
        return hm

    def create_gv_node(self, node_name: str, title: str, opt_color=None):
        display_title = title.replace("_", ".")
        titled_table = HtmlMatrix.create_titled_table(display_title, self.build())
        col_str = "" if opt_color is None else f" color={opt_color}"
        return f"\"{node_name}\" [label = <{titled_table.build()}>; shape=plaintext;{col_str}];"


class HtmlCompareTable(HtmlMatrix):
    F_FMT = r"{:.9g}"
    D_FMT = r"{:.17g}"
    ULP_DIFF_FMT = r"{:.2g}"

    def __init__(self, n_rows, n_cols):
        super().__init__(3, 2)
        for i, row_name in enumerate(('float', 'double', 'diff')):
            super().set_entry(i, 0, row_name)
            super().set_entry(i, 1, HtmlMatrix(n_rows, n_cols))

    def set_entry(self, r, c, df_row: pd.Series):
        self._str_entries[0][1].set_entry(r, c, self.F_FMT.format(df_row["float_val"]))
        self._str_entries[1][1].set_entry(r, c, self.D_FMT.format(df_row["double_val"]))
        if df_row["ulp_diff"] < 100000:
            diff_str_entry = "{:.2g} ULPs".format(df_row["ulp_diff"])
        else:
            diff_str_entry = "{:.2g} abs".format(df_row["abs_diff"])
        self._str_entries[2][1].set_entry(r, c, diff_str_entry)

        if df_row["ulp_diff"] > 0.5:
            self.set_entry_high_err(r, c)

    def set_entry_high_err(self, r, c):
        for row in self._str_entries:
            row[1].set_entry_high_err(r, c)

    def dims(self):
        return self._str_entries[0][1].dims()


class GvBuilder:

    def __init__(self):
        self.lines = [
            "graph [label = \"\"];",
            "node [shape = record];",
        ]

    def append_vec_node(self, df: pd.DataFrame, df_vec_name: str, opt_color=None, opt_override_name=None):
        table = HtmlCompareTable(1, 3)
        for i in range(table.dims()[1]):
            df_row_name = f"{df_vec_name}_{'xyz'[i]}"
            table.set_entry(0, i, df.loc[df_row_name])
        self.lines.append(table.create_gv_node(
            df_vec_name,
            df_vec_name if opt_override_name is None else opt_override_name,
            opt_color,
        ))

    def append_mat_node(self, df: pd.DataFrame, df_mat_name: str, n_rows, n_cols, opt_color=None, opt_override_name=None):
        table = HtmlCompareTable(n_rows, n_cols)
        for r in range(n_rows):
            for c in range(n_cols):
                df_row_name = f"{df_mat_name}_{r * n_cols + c}"
                table.set_entry(r, c, df.loc[df_row_name])
        self.lines.append(table.create_gv_node(
            df_mat_name,
            df_mat_name if opt_override_name is None else opt_override_name,
            opt_color,
        ))

    def append_plane_node(self, df: pd.DataFrame, df_plane_name: str, opt_color=None):
        table = HtmlCompareTable(1, 4)
        for i in range(table.dims()[1]):
            df_row_name = f"{df_plane_name}_{['n_x', 'n_y', 'n_z', 'd'][i]}"
            table.set_entry(0, i, df.loc[df_row_name])
        self.lines.append(table.create_gv_node(df_plane_name, df_plane_name, opt_color))

    def append_chain(self, df: pd.DataFrame):
        for is_blue in (True, False):
            portal_color = bool_to_portal_col(is_blue)
            gv_color = bool_to_gv_col(is_blue)

            # pos/ang
            self.append_vec_node(df, f"{portal_color}_pos", gv_color)
            self.append_vec_node(df, f"{portal_color}_ang", gv_color)

            # f/r/u vectors
            self.lines.append(f"{portal_color}_AngleVectors [label=AngleVectors];")
            self.lines.append(f"{portal_color}_ang -> {portal_color}_AngleVectors;")
            for local_dir in "fru":
                df_local_dir_row = portal_color + "_" + local_dir
                self.append_vec_node(df, df_local_dir_row, gv_color)
                self.lines.append(f"{portal_color}_AngleVectors -> {df_local_dir_row};")

            # portal ent matrix
            self.lines.append(f"{portal_color}_AngleMatrix [label=AngleMatrix];")
            self.lines.append(
                f"{{{portal_color}_ang {portal_color}_pos}} -> {portal_color}_AngleMatrix;"
            )
            self.append_mat_node(df, portal_color + "_mat", 3, 4, gv_color)
            self.lines.append(f"{portal_color}_AngleMatrix -> {portal_color}_mat;")

            # portal planes
            self.append_plane_node(df, f"{portal_color}_plane", gv_color)
            self.lines.append(f"{{{portal_color}_f {portal_color}_pos}} -> {portal_color}_plane")

            # teleport matrices
            self.append_mat_node(df, f"{portal_color}_tp_mat", 4, 4,
                                 gv_color, f"{portal_color} teleport mat")

            self.lines.append(
                f"{{{portal_color}_mat {opposite_portal_col(portal_color)}_mat}} -> {portal_color}_tp_mat"
            )

        # entity positions
        tp_idx = 0
        while f"ent_pre_teleport_{tp_idx}_x" in df.index:

            is_blue = df.loc[f"ent_pre_teleport_{tp_idx}_x"]["is_blue"]
            ent_node_color = "green"

            if tp_idx == 0:
                self.append_vec_node(
                    df, f"ent_pre_teleport_{tp_idx}", ent_node_color, f"ents[{tp_idx}].center"
                )
            self.append_vec_node(
                df, f"ent_post_teleport_{tp_idx}", ent_node_color, f"ents[{tp_idx + 1}].center"
            )
            if tp_idx == 0:
                from_node = f"ent_pre_teleport_{tp_idx}"
                to_node = f"ent_post_teleport_{tp_idx}"
            else:
                from_node = f"ent_post_teleport_{tp_idx - 1}"
                to_node = f"ent_post_teleport_{tp_idx}"

            self.lines.append(
                f"Teleport_{tp_idx} [label=Teleport; style=filled; fillcolor={bool_to_gv_col(is_blue)}];")
            self.lines.append(f"{from_node} -> Teleport_{tp_idx};")
            self.lines.append(f"Teleport_{tp_idx} -> {to_node};")
            self.lines.append(f"{'blue' if is_blue else 'orange'}_tp_mat -> Teleport_{tp_idx}")

            tp_idx += 1

    def build(self, indent_str: str) -> str:
        out_str = "/* Generated by Monocle */\n"
        out_str += "digraph G {\n"
        for line in self.lines:
            out_str += indent_str
            out_str += line
            out_str += '\n'
        out_str += '}\n'
        return out_str


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('path_to_chain_csv_dump')
    args = parser.parse_args()

    csv_path = os.path.realpath(args.path_to_chain_csv_dump)
    print("Loading", csv_path)
    df = pd.read_csv(
        csv_path,
        dtype={
            'name': 'str',
            'float': np.float32,
            'double': np.float64,
            'ulp_diff': np.float64,
        },
        index_col="name"
    )

    gv = GvBuilder()
    gv.append_chain(df)
    out_gv_path = os.path.splitext(csv_path)[0] + ".gv"
    with open(out_gv_path, 'wt') as f:
        f.write(gv.build('    '))
    print("Wrote graphviz output to", out_gv_path)
