MVID=/mnt/c/Users/alex/mvid2.0
rm -f $MVID/public/workers/WasmEncoder1t.*
rm -f $MVID/src/editor/export/WasmEncoder1t.*
sed -i '1 i/* eslint-disable */' WasmEncoder1t.js
sed -i "s/global.performance = require('perf_hooks').performance;/ /g" WasmEncoder1t.js

cp WasmEncoder1t.* $MVID/public/workers/
cp WasmEncoder1t.js $MVID/src/editor/export

echo copied to $MVID