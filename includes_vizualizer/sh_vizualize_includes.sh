(
  cd ../src/
  perl ../includes_vizualizer/cinclude2dot.pl > in.dot
  dot -Tpng in.dot > out_includes_graph.png
  rm in.dot
  mv out_includes_graph.png ../includes_vizualizer
)
