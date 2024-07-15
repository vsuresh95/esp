# Software
echo ""
echo "Mini-ERA: Software"
echo ""
./miniera/miniera-sw.exe -t ./miniera/traces/test_trace1.new -R ./miniera/traces/norm_radar_01k_dictionary.dfn -V ./miniera/traces/vit_dictionary.dfn

# OS
echo ""
echo "Mini-ERA: OS"
echo ""
./miniera/miniera-os.exe -t ./miniera/traces/test_trace1.new -R ./miniera/traces/norm_radar_01k_dictionary.dfn -V ./miniera/traces/vit_dictionary.dfn

# MESI
echo ""
echo "Mini-ERA: MESI"
echo ""
./miniera/miniera-mesi.exe -t ./miniera/traces/test_trace1.new -R ./miniera/traces/norm_radar_01k_dictionary.dfn -V ./miniera/traces/vit_dictionary.dfn

# DMA
echo ""
echo "Mini-ERA: DMA"
echo ""
./miniera/miniera-dma.exe -t ./miniera/traces/test_trace1.new -R ./miniera/traces/norm_radar_01k_dictionary.dfn -V ./miniera/traces/vit_dictionary.dfn

