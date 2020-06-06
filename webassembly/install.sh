rm -f /home/alex/prog/musicvid.org/public/workers/WasmEncoder1t.*
rm -f /home/alex/prog/musicvid.org/src/editor/export/WasmEncoder1t.*
sed -i '1 i/* eslint-disable */' WasmEncoder1t.js
sed -i "s/global.performance = require('perf_hooks').performance;/ /g" WasmEncoder1t.js

cp WasmEncoder1t.* /home/alex/prog/musicvid.org/public/workers/
cp WasmEncoder1t.js /home/alex/prog/musicvid.org/src/editor/export
