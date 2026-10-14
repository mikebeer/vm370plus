public class hello {
    public static void main(String[] a) throws Exception {
        System.out.println("Hello from Java on CMS, VM/370plus");
        System.out.println("java.vm.name=" + System.getProperty("java.vm.name") + " os.name=" + System.getProperty("os.name"));
        long s = 0; for (int i = 1; i <= 100000; i++) s += i;
        System.out.println("sum=" + s + " sqrt2=" + Math.sqrt(2.0));
        Thread t = new Thread(new Runnable() { public void run() { System.out.println("thread says hi"); } });
        t.start(); t.join();
        java.util.HashMap<String,Integer> m = new java.util.HashMap<String,Integer>();
        for (String w : "the quick brown fox jumps over the lazy dog the end".split(" ")) m.put(w, m.containsKey(w) ? m.get(w) + 1 : 1);
        System.out.println("words=" + new java.util.TreeMap<String,Integer>(m));
        java.io.PrintWriter w = new java.io.PrintWriter(new java.io.FileWriter("/javaout.txt"));
        w.println("written by Java"); w.close();
        java.io.BufferedReader r = new java.io.BufferedReader(new java.io.FileReader("/javaout.txt"));
        System.out.println("read back: " + r.readLine()); r.close();
        System.gc();
        System.out.println("free=" + Runtime.getRuntime().freeMemory() + " total=" + Runtime.getRuntime().totalMemory());
    }
}
