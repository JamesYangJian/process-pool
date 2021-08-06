import sys
import os


def merge_csv_files(input_csv_files, target_file):
    try:
        with open(target_file, 'wb') as ostream:
            for f in input_csv_files:
                with open(f, 'rb') as istream:
                    for line in istream:
                        ostream.write(line)
        return True
    except Exception as e:
        print("Error! Merge csv file failed, reason:%s" % str(e))
        return False


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 %s input_dir output_file\n" % __file__)
        sys.exit(0)

    input_dir = sys.argv[1]
    output_file = sys.argv[2]

    tmp_csv_files = os.listdir(input_dir)
    tmp_csv_files = list(map(lambda x: input_dir+'/'+x, tmp_csv_files))

    merge_csv_files(tmp_csv_files, output_file)

    print("Finished!")