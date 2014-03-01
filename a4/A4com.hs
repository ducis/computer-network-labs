module A4com where
run file func = rdnam file >>= return.func -- example: run "task1_out.nam" task1_tcp

rdnam::String->IO [[String]]
rdnam fname = readFile fname >>= return.map words.lines

smplpred cmplist wds = and $ map (\(i,w)->wds !! i==w) cmplist
smplfilt cmplist = filter (smplpred cmplist)

sumtr term_index_in_line = sum . map (\wds->(read $ wds !! term_index_in_line)::Int)

task1_tcpftp::[[String]]->Int
task1_tcpftp = sumtr 10 . smplfilt [(0,"r"),(4,"2"),(6,"3"),(16,"1"),(8,"tcp")]

task1_udpcbr = sumtr 10 . smplfilt [(0,"r"),(4,"2"),(6,"3"),(16,"2"),(8,"cbr")]


