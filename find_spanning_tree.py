#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["folium", "networkx", "pandas", "branca"]
# ///
import os
import subprocess
import sys

import branca.colormap as cm
import folium
import networkx as nx
import pandas as pd

INPUT_FILE = "data/buildings.csv"
OUTPUT_FILE = "data/spanning_tree.csv"
C_EXECUTABLE = "./spanning_tree"
MAP_FILE = "route_map.html"
CITY = "Санкт-Петербург"  

# Цвета компонент связности
PALETTE = ["#d62728", "#1f77b4", "#2ca02c", "#9467bd", "#ff7f0e", "#17becf",
           "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#393b79", "#637939"]


def run_c_processor() -> bool:
    """Запускает C-программу. Старый результат удаляем, чтобы не рисовать устаревшие данные."""
    if not os.path.isfile(C_EXECUTABLE):
        print(f"Не найден {C_EXECUTABLE}. Скомпилируйте: make spanning_tree")
        return False

    if os.path.isfile(OUTPUT_FILE):
        os.remove(OUTPUT_FILE)

    print("Считаем остовное дерево (может занять несколько минут)...")
    try:
        result = subprocess.run([C_EXECUTABLE], capture_output=True, text=True,
                                check=True, timeout=1800)
    except subprocess.TimeoutExpired:
        print("C-программа работает слишком долго. Запустите ./spanning_tree вручную.")
        return False
    except subprocess.CalledProcessError as e:
        print(f"Ошибка C-программы: {e}\n{e.stderr}")
        return False

    print(result.stdout.strip())
    return os.path.isfile(OUTPUT_FILE)


def build_map(df_in: pd.DataFrame, df_out: pd.DataFrame) -> tuple[folium.Map, dict]:
    # Узлы: id -> (lat, lon, address)
    nodes = df_in.drop_duplicates(subset=["id"]).copy()
    nodes["id"] = nodes["id"].astype(int)
    if "address" not in nodes:
        nodes["address"] = ""
    nodes["address"] = nodes["address"].fillna("").astype(str)
    pos = {r.id: (r.lat, r.lon, r.address) for r in nodes.itertuples()}

    # Граф остова и его компоненты связности
    edges = list(zip(df_out["node_id_from"].astype(int),
                     df_out["node_id_to"].astype(int),
                     df_out["length"]))
    G = nx.Graph()
    G.add_nodes_from(pos.keys())
    G.add_weighted_edges_from(edges)

    components = list(nx.connected_components(G))
    comp_of = {}
    for k, comp in enumerate(components):
        for node in comp:
            comp_of[node] = k

    # Цвет ребра по длине: короткие синие, длинные красные
    lengths = df_out["length"]
    colormap = cm.LinearColormap(["#2c7bb6", "#ffffbf", "#d7191c"],
                                 vmin=lengths.min(), vmax=lengths.max())
    colormap.caption = "Длина ребра, м"

    edge_features = []
    for a, b, length in edges:
        lat1, lon1, _ = pos[a]
        lat2, lon2, _ = pos[b]
        edge_features.append({
            "type": "Feature",
            "geometry": {"type": "LineString",
                         "coordinates": [[lon1, lat1], [lon2, lat2]]},
            "properties": {"length": round(float(length), 1)},
        })

    # Здания, попавшие в остов
    node_features = []
    for nid in G.nodes:
        if G.degree(nid) == 0:
            continue
        lat, lon, addr = pos[nid]
        c = comp_of[nid]
        node_features.append({
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": [lon, lat]},
            "properties": {
                "address": addr or "(без адреса)",
                "component": int(c),
                "color": PALETTE[c % len(PALETTE)],
            },
        })

    center = (nodes["lat"].mean(), nodes["lon"].mean())

    # Без подложки: OpenStreetMap блокирует тайлы для файлов, открытых с диска
    m = folium.Map(location=center, zoom_start=13, tiles=None)
    m.get_root().header.add_child(folium.Element(
        "<style>.leaflet-container{background:#f2f2ef !important;}</style>"
    ))

    folium.GeoJson(
        {"type": "FeatureCollection", "features": edge_features},
        name="Остовное дерево",
        style_function=lambda f: {
            "color": colormap(f["properties"]["length"]),
            "weight": 3,
            "opacity": 0.9,
        },
        tooltip=folium.GeoJsonTooltip(fields=["length"], aliases=["Длина, м"]),
    ).add_to(m)

    folium.GeoJson(
        {"type": "FeatureCollection", "features": node_features},
        name="Здания",
        marker=folium.CircleMarker(radius=3),
        style_function=lambda f: {
            "fillColor": f["properties"]["color"],
            "color": f["properties"]["color"],
            "weight": 0,
            "fillOpacity": 0.9,
        },
        popup=folium.GeoJsonPopup(fields=["address", "component"],
                                  aliases=["Адрес", "Компонента"]),
    ).add_to(m)

    colormap.add_to(m)

    # Статистика: кусков связности считаем только из тех, где больше одного здания
    stats = {
        "buildings": len(nodes),
        "edges": len(df_out),
        "length_km": df_out["length"].sum() / 1000,
        "pieces": sum(1 for comp in components if len(comp) > 1),
    }
    card = f"""
    <div style="position:fixed; bottom:24px; left:24px; z-index:9999;
                background:rgba(255,255,255,0.92); color:#222; padding:14px 18px;
                border-radius:12px; font-family:sans-serif; font-size:14px;
                box-shadow:0 2px 12px rgba(0,0,0,.2); line-height:1.6;">
      <b style="font-size:16px">{CITY}</b><br>
      Зданий: {stats['buildings']}<br>
      Рёбер в остове: {stats['edges']}<br>
      Суммарная длина: {stats['length_km']:.2f} км<br>
      Кусков связности: {stats['pieces']}
    </div>"""
    m.get_root().html.add_child(folium.Element(card))

    folium.LayerControl().add_to(m)
    return m, stats


def main():
    print("Поиск остовного дерева")
    print("=" * 50)

    if not run_c_processor():
        sys.exit(1)

    df_in = pd.read_csv(INPUT_FILE, sep="\t")
    df_out = pd.read_csv(OUTPUT_FILE, sep="\t", header=None,
                         names=["node_id_from", "node_id_to", "length"])
    if df_out.empty:
        print("Остовное дерево пустое, рисовать нечего")
        sys.exit(1)

    m, stats = build_map(df_in, df_out)
    m.save(MAP_FILE)

    print(f"Карта сохранена: {MAP_FILE}")
    print(f"Зданий: {stats['buildings']}, рёбер в остове: {stats['edges']}, "
          f"суммарная длина: {stats['length_km']:.2f} км, "
          f"кусков связности: {stats['pieces']}")


if __name__ == "__main__":
    main()