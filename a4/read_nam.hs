module Read_nam where
rdnam fname = readFile fname >>= return.map words.lines
