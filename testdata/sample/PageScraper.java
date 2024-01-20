import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URI;
import java.net.http.*;
import java.util.*;
import java.util.function.Function;
import java.util.stream.Collectors;

public class PageScraper {

    public static void main(final String[] args) throws Exception {
        final String[] urls = {
                "https://de.wikipedia.org/wiki/Technische_Universit%C3%A4t_Dresden",
                "https://en.wikipedia.org/wiki/TU_Dresden",
                "https://da.wikipedia.org/wiki/Technische_Universit%C3%A4t_Dresden",
                "https://es.wikipedia.org/wiki/Universidad_Polit%C3%A9cnica_de_Dresde",
                "https://fr.wikipedia.org/wiki/Universit%C3%A9_technique_de_Dresde",
                "https://it.wikipedia.org/wiki/Universit%C3%A0_tecnica_di_Dresda",
                "https://nl.wikipedia.org/wiki/Technische_Universiteit_Dresden",
                "https://pl.wikipedia.org/wiki/Uniwersytet_Techniczny_w_Dre%C5%BAnie",
                "https://ru.wikipedia.org/wiki/%D0%94%D1%80%D0%B5%D0%B7%D0%B4%D0%B5%D0%BD%D1%81%D0%BA%D0%B8%D0%B9_%D1%82%D0%B5%D1%85%D0%BD%D0%B8%D1%87%D0%B5%D1%81%D0%BA%D0%B8%D0%B9_%D1%83%D0%BD%D0%B8%D0%B2%D0%B5%D1%80%D1%81%D0%B8%D1%82%D0%B5%D1%82",
                "https://zh.wikipedia.org/wiki/%E5%BE%B7%E7%B4%AF%E6%96%AF%E9%A1%BF%E5%B7%A5%E4%B8%9A%E5%A4%A7%E5%AD%A6",
                "https://pt.wikipedia.org/wiki/Universidade_T%C3%A9cnica_de_Dresden"
        };

        final long start = System.currentTimeMillis();

        int longestChars = -1;
        String longest = null;

        final Map<Character, Long> allChars = new HashMap<>();
        final Map<String, Long> allWords = new HashMap<>();

        for (final String url : urls) {
            final String body = getPage(url);

            if (body == null)
                continue;

            System.out.printf("Running %s\n", url);

            final Map<Character, Long> chars = countChars(body);
            final Map<String, Long> words = countStrings(body);

            if (body.length() > longestChars) {
                longestChars = body.length();
                longest = url;
            }

            for (final Map.Entry<Character, Long> entry : chars.entrySet()) {
                long value = allChars.getOrDefault(entry.getKey(), 0L);
                value += entry.getValue();
                allChars.put(entry.getKey(), value);
            }

            for (final Map.Entry<String, Long> entry : words.entrySet()) {
                long value = allWords.getOrDefault(entry.getKey(), 0L);
                value += entry.getValue();
                allWords.put(entry.getKey(), value);
            }

            Thread.sleep(500);
        }

        System.out.printf("Longest page %s\n", longest);

        final List<Map.Entry<Character, Long>> topFiveCharacters = getTop(allChars, 5);
        System.out.printf("Global most used chars %s\n", topFiveCharacters);

        final List<Map.Entry<String, Long>> topFiveWords = getTop(allWords, 5);
        System.out.printf("Global most used words %s\n", topFiveWords);

        writeMapToFile(allChars, "allchars.txt");
        writeMapToFile(allWords, "allwords.txt");

        final long end = System.currentTimeMillis();

        System.out.printf("Elapsed: %dms\n", end - start);
    }

    private static String getPage(final String url) {
        final HttpClient client = HttpClient.newHttpClient();

        try {
            final URI uri = new URI(url);
            final HttpRequest request = HttpRequest.newBuilder()
                    .uri(uri)
                    .build();

            final HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
            return response.body();
        } catch (Exception ex) {
            System.err.println(ex.getMessage());
            return null;
        }
    }

    private static Map<Character, Long> countChars(final String body) {
        final Map<Character, Long> chars = body.chars()
                .mapToObj(i -> (char) i)
                .collect(Collectors.groupingBy(Function.identity(), Collectors.counting()));

        System.out.printf("Number of unique chars: %d\n", chars.size());

        final List<Map.Entry<Character, Long>> topFiveCharacters = getTop(chars, 5);
        System.out.printf("Most used chars %s\n", topFiveCharacters);

        return chars;
    }

    private static Map<String, Long> countStrings(final String body) {
        final Map<String, Long> words = Arrays.stream(body.split("\\s+"))
                .collect(Collectors.groupingBy(Function.identity(), Collectors.counting()));

        System.out.printf("Number of unique words: %d\n", words.size());

        final List<Map.Entry<String, Long>> topFiveWords = getTop(words, 5);
        System.out.printf("Most used words %s\n", topFiveWords);

        return words;
    }

    private static <T> List<Map.Entry<T, Long>> getTop(final Map<T, Long> allWords, final int amount) {
        return allWords.entrySet().stream()
                .sorted(Map.Entry.<T, Long>comparingByValue().reversed())
                .limit(amount)
                .collect(Collectors.toList());
    }

    private static <K extends Comparable<? super K>, V extends Comparable<? super V>> void writeMapToFile(final Map<K, V> map, final String filename) {
        final Map<K, V> sorted = map.entrySet()
                .stream()
                .sorted(Map.Entry.<K, V>comparingByValue().reversed()
                        .thenComparing(Map.Entry.comparingByKey()))
                .collect(Collectors.toMap(
                        Map.Entry::getKey,
                        Map.Entry::getValue,
                        (e1, e2) -> e1,
                        LinkedHashMap::new));

        try (final BufferedWriter writer = new BufferedWriter(new FileWriter(filename))) {
            for (final Map.Entry<K, V> entry : sorted.entrySet()) {
                writer.write(entry.getKey() + " = " + entry.getValue());
                writer.newLine();
            }
        } catch (IOException ex) {
            System.err.println(ex.getMessage());
        }
    }
}