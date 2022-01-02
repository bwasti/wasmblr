      function remove(elem) {
          while (elem.firstChild) {
              elem.removeChild(elem.firstChild);
          }
      }

      function error() {
          const out = document.querySelector("#graph");
          let b = document.createElement('b');
          b.appendChild(document.createTextNode("Cannot run SIMD on this browser.  Please try Chrome or Firefox (or refresh to try again)"));
          out.appendChild(b);
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createTextNode("To run a non-SIMD variant, click here:"));
              out.appendChild(document.createElement('br'));
              let a = document.createElement('a');
              a.href = "https://bwasti.github.io/wasmblr/index_no_simd.html";
              a.textContent = "Non-SIMD benchmark";
              out.appendChild(a);
          setTimeout(function() {
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createTextNode("To run in node.js, please grab the code here:"));
              out.appendChild(document.createElement('br'));
              let a = document.createElement('a');
              a.href = "https://github.com/bwasti/wasmblr/tree/main/emscripten_example";
              a.textContent = "https://github.com/bwasti/wasmblr/tree/main/emscripten_example";
              out.appendChild(a);
          }, 500);

          function addImg(url) {
              const image = document.createElement('img');
              image.src = url;
              image.style.width = "100%";
              image.style.maxWidth = "800px";
              out.appendChild(image);
          }
          setTimeout(function() {
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createTextNode("Alternatively, here are sample results in Firefox:"));
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createElement('br'));
              addImg("https://i.imgur.com/rIjkz6D.png");
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createTextNode("and Chrome:"));
              out.appendChild(document.createElement('br'));
              out.appendChild(document.createElement('br'));
              addImg("https://i.imgur.com/0VA6ioi.png");
              out.appendChild(document.createElement('br'));
          }, 1000);
      }

      function buildGraph(times) {
          var margin = {
                  top: 10,
                  right: 40,
                  bottom: 30,
                  left: 40
              },
              width = Math.floor(document.body.clientWidth * 0.8) - margin.left - margin.right,
              height = 400 - margin.top - margin.bottom;
          d3.select("svg").remove();
          let g = document.querySelector("#graph");
          remove(g);
          var res = Object.keys(times);
          var color = d3.scaleOrdinal().domain(res).range(["#e41a1c", "#377eb8", "#4daf4a", "#984ea3", "#ff7f00", "#ffff33", "#a65628", "#f781bf", "#999999"]);
          let keys = Object.keys(times);
          keys.sort();
          for (let k of keys) {
              let span = document.createElement("span");
              span.style.color = color(k);
              span.style.padding = "5px";
              span.textContent = k;
              g.appendChild(span);
          }
          g.appendChild(document.createElement("br"));
          var svg = d3
              .select("#graph")
              .append("svg")
              .attr("width", width + margin.left + margin.right)
              .attr("height", height + margin.top + margin.bottom)
              .append("g")
              .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

          let d = [];
          let all_data = [];
          for (let k of res) {
              d.push({
                  key: k,
                  values: times[k],
              });
              all_data = all_data.concat(times[k]);
          }

          var x = d3.scaleLog().base(2).domain(
                  d3.extent(all_data, function(d) {
                      return d[0];
                  }))
              .range([0, width]);
          svg
              .append("g")
              .attr("transform", "translate(0," + height + ")")
              .call(d3.axisBottom(x).ticks(25));

          // text label for the x axis
          svg
              .append("text")
              .attr("transform", "translate(" + width / 2 + " ," + (height + margin.top + 20) + ")")
              .style("text-anchor", "middle")
              .text("Vector Size");

          var y = d3
              .scaleLinear()
              .domain([
                  0,
                  d3.max(all_data, function(d) {
                      return d[1];
                  }),
              ])
              .range([height, 0]);
          svg.append("g").call(d3.axisLeft(y));

          // text label for the y axis
          svg
              .append("text")
              .attr("transform", "rotate(-90)")
              .attr("y", 0 - margin.left)
              .attr("x", 0 - height / 2)
              .attr("dy", "1em")
              .style("text-anchor", "middle")
              .text("GB/s");

          let lines = svg.selectAll(".line").data(d);

          lines
              .enter()
              .append("path")
              .attr("fill", "none")
              .attr("stroke", function(d) {
                  return color(d.key);
              })
              .attr("stroke-width", 1.5)
              .attr("d", function(d) {
                  return d3
                      .line()
                      .x(function(d) {
                          return x(d[0]);
                      })
                      .y(function(d) {
                          return y(d[1]);
                      })(d.values);
              })
              .merge(lines)
              .transition()
              .duration(500);
      }

      function init(Module) {
          Module._jit_add(4, 1);
          let g = document.querySelector("#graph");
          remove(g);
          let button = document.createElement("button");
          button.textContent = "Run Benchmarks";
          button.addEventListener("click", function() {
              let pre = document.querySelector("pre");
              remove(pre);
              let times = {};
              let keys = ["Pure JS", "Pure JS (unrolled)", "TypedArrays", "Emscripten (simd)", "Wasmblr", "Wasmblr (tuned)"];
              for (let k of keys) {
                  times[k] = [];
              }
              (async () => {
                  for (let i = 2; i < 20; ++i) {
                      const result = await benchmark(Module, Math.pow(2, i));
                      for (let k of keys) {
                          times[k].push([Math.pow(2, i), result[keys.indexOf(k)]]);
                      }
                      buildGraph(times);
                  }
              })();
          });
          document.getElementById("button_div").appendChild(button);
      }

      window.addEventListener("load", function() {
          try {
              createMyModule().then(init).catch(e => {
                  setTimeout(error, 500);
              });
          } catch (e) {
              setTimeout(error, 500);
          };
      });
