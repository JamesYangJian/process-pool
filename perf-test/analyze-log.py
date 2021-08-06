import pandas as pd
import numpy as np
import argparse
import os
import sys
import shutil
import json

def create_dir(path):
    if os.path.exists(path):
        try:
            shutil.rmtree(path)
        except Exception as e:
            pass
    try:
        os.mkdir(path)
    except Exception as e:
        print("Fatal: can't create directory: %s" % path)
        sys.exit(-1)

def parse_total_url_by_duration(url_info):
    urls = url_info.keys()
    new_dfs = []

    for url in urls:
        df_info = url_info[url].sort_values("duration")[-1:]
        new_dfs.append(df_info)

    new_df = pd.concat(new_dfs).sort_values("duration")
    return new_df

def parse_single_url(url, url_df, total_count):
    url_stat = {}
    url_stat['count'] = len(url_df)
    url_stat['usage'] = round (len(url_df) / total_count, 6)
    url_stat['duration_min'] = round(url_df.duration.min(), 3)
    url_stat['duration_max'] = round(url_df.duration.max(), 3)
    url_stat['duration_mean'] = round(url_df.duration.mean(), 3)
    url_stat['duration_std'] = round(url_df.duration.std(), 3)
    url_stat['p95'] = round(url_df.p95.get(url_df.index.values.tolist()[0]), 3)
    url_stat['p99'] = round(url_df.p99.get(url_df.index.values.tolist()[0]), 3)

    return url_stat

def main(csv_file, count, stat):
    df = pd.read_csv(csv_file,
                     sep=',',
                     dtype={
                        'timestamp': str,
                        'method': str,
                        'status': str,
                        'duration': np.float64
                     }
                )

    total_count = len(df)
    df_grp = df.groupby('method')
    url_info = {}
    url_usage = {}
    for n, g in df_grp:
        g['p95'] = g.duration.quantile(0.95)
        g['p99'] = g.duration.quantile(0.99)
        url_info[n] = g
        url_usage[n] = len(g)

    new_df = parse_total_url_by_duration(url_info)
    if count > len(new_df):
        count = len(new_df)
    stat_info = {}

    if stat.lower() == 'duration':
        top_df = new_df[-count:]
        top_index = top_df.index.values.tolist()
        for i in range(len(top_index)-1, -1, -1):
            url = top_df['method'].get(top_index[i])
            url_stat = parse_single_url(url, url_info[url], total_count)
            stat_info[url] = url_stat
    elif stat == 'usage':
        sorted_url_usage = sorted(url_usage.items(), key=lambda obj:obj[1], reverse=True)
        for i in range(count):
            url = sorted_url_usage[i][0]
            url_stat = parse_single_url(url, url_info[url], total_count)
            stat_info[url] = url_stat


    stat_string = json.dumps(stat_info, indent=4)
    output_path = './output-{stat}'.format(stat=stat)
    create_dir(output_path)

    # write stat_info into stat_info.json
    stat_file = output_path + '/stat_info-{usage}.csv'.format(usage=stat)
    header = "url,count,usage,duration_min, duration_max, duration_mean, p95, p99, duration_std\n"
    fmt = "{url},{count},{usage},{duration_min},{duration_max},{duration_mean},{p95},{p99},{duration_std}\n"
    with open(stat_file, "w") as of:
        of.write(header)
        for url in stat_info.keys():
            info = stat_info[url]
            v_str = fmt.format(url=url,count=info['count'], usage=info['usage'],
                                duration_min=info['duration_min'],
                                duration_max=info['duration_max'],
                                duration_mean=info['duration_mean'],
                                p95 = info['p95'],
                                p99 = info['p99'],
                                duration_std=info['duration_std'])

            of.write(v_str)

    if stat == 'usage':
        sorted_list = sorted(stat_info.items(), key=lambda x:x[1]['duration_max'], reverse=True)
        stat_sorted_file = output_path + '/stat_info-{usage}-sorted.csv'.format(usage=stat)
        with open(stat_sorted_file, 'w') as of:
            of.write(header)
            for i in range(len(sorted_list)):
                item = sorted_list[i]
                v_str = fmt.format(url=item[0],count=item[1]['count'], usage=item[1]['usage'],
                                    duration_min=item[1]['duration_min'],
                                    duration_max=item[1]['duration_max'],
                                    duration_mean=item[1]['duration_mean'],
                                    p95 = item[1]['p95'],
                                    p99 = item[1]['p99'],
                                    duration_std=item[1]['duration_std'])
                of.write(v_str)

    return stat_info, url_info

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Analyze OneEye logs')
    parser.add_argument('-f', '--file', help = 'One-eye log file in csv format.')
    parser.add_argument('-c', '--count', default=50, help = 'Count of top URL to analyze.')
    parser.add_argument('-s', '--stat', default='Duration', help = 'Method of statistic (Duration|Usage')

    args = parser.parse_args()

    if not args.file:
        print("Please specify the log file to analyze!")
        sys.exit(1)

    main(args.file, int(args.count), args.stat)
