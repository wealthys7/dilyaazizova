# https://pypi.yandex.ru/simple/
# https://pypi.tuna.tsinghua.edu.cn/simple/

from typing import Literal
import osmnx as ox
import networkx as nx
import csv

# Настройка логирования (чтобы видеть прогресс загрузки)
ox.settings.timeout: Literal[3600] = 60 * 60  # sec
ox.settings.log_console = True
ox.settings.use_cache = True  # Кэшировать запросы, чтобы не качать повторно
ox.settings.overpass_endpoint = "https://overpass.kumi.systems/api/"
# Или альтернатива: "https://overpass.osm.ch/api/"

buildings = None
while buildings is None:
    city: str = input("Введите название города: ")

    gdf = ox.geocode_to_gdf(city)

    if gdf.shape[0] == 0:
        print("Город не найден, попробуйте уточнить название города...")
        continue

    official_name = gdf.iloc[0]['name']
    print(f"  Место найдено!")
    print(f"   Официальное название: {official_name}")
    print(f"   Площадь: {gdf.iloc[0].geometry.area:.2f} кв. ед.")

    approve: str = input("Нажмите 'y', чтобы подтвердить что найденный город этот тот, который вы искали: ")
    if approve.lower() == 'y':
        tags = {
            'building': ['residential', 'commercial', 'office', 'apartments', 'house']
        }
        buildings = ox.features_from_polygon(gdf.iloc[0].geometry, tags=tags)
        print(f"  Загружено зданий: {len(buildings)}")
    else:
        print("Видимо, нашелся не тот город. Попробуйте уточнить название города")

print(f"  Отлично, нашлось {buildings.shape[0]} зданий по вашему запросу. Дальше они будут сохранены в файл data/buildings.csv")

df = buildings.copy()

# Преобразуем геометрию в строку WKT
buildings['centroid'] = buildings.geometry.centroid
buildings['lat'] = buildings.centroid.y  # Широта
buildings['lon'] = buildings.centroid.x  # Долгота
# df['geometry'] = df['geometry'].apply(lambda x: x.wkt)
buildings['address'] = buildings['addr:street'].str.cat(
    buildings['addr:housenumber'],
    sep=', ', 
    na_rep=''
).str.strip(', ')

# Сохраняем в CSV
buildings = buildings[['address', 'lat', 'lon']].reset_index(drop=False)
print(buildings.columns.tolist())
buildings = buildings.drop(columns=['element'])
# buildings = buildings.rename(columns={'element	id', })
buildings.to_csv("data/buildings.csv", index=False, encoding="utf-8-sig", sep='\t')
